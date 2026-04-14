# 自定义 IDF 4.4 框架包构建记录

> 本文记录为 eink-ra01s-gp02 构建含 `CONFIG_PM_ENABLE=y` 的自定义 framework 包的
> 完整过程，包括调查路径、意外失败及修复决策。
>
> 框架三层结构、仓库关系图、单包/双包架构差异详见
> [1\_\_CUSTOM_SDK_NOTES.md](./1__CUSTOM_SDK_NOTES.md)，本文不重复这些内容。

---

## 第 1 章：背景与约束

目标：在 Meshtastic（IDF 4.4 环境）中让 `esp_pm_configure()` 真正生效，使 CPU
在 FreeRTOS idle 时自动降频（DFS），降低 eink-ra01s-gp02 的平均功耗。

核心约束：

- Meshtastic 固定使用 `platform = platformio/espressif32@6.13.0`
  （arduino-esp32 2.x / IDF 4.4），不能随意升级
- 大量第三方库（NimBLE 1.4.3、libpax 等）已固定在 IDF 4.4 兼容版本，升级
  IDF 版本会引发连锁不兼容
- **默认的 arduino-esp32 2.0.17 框架包未编译进 `CONFIG_PM_ENABLE=y`**，
  调用 `esp_pm_configure()` 返回 `ESP_ERR_NOT_SUPPORTED` (0x102)，不产生任何效果
- 唯一路径：替换框架包中的预编译 IDF 静态库（注入 PM_ENABLE），arduino 源码、
  工具链、所有第三方库保持不变

---

## 第 2 章：版本链溯源

从 `platform = platformio/espressif32@6.13.0` 出发，共三跳，确认需要拉取哪个
lib-builder 分支。每一跳都有本地文件作为证据。

### 2.1 第一跳：platform → arduino-esp32 版本号

PlatformIO 平台安装后描述文件的位置：

```
~/.platformio/platforms/espressif32@6.13.0/platform.json
```

其中 framework 包的字段：

```json
"framework-arduinoespressif32": {
    "type": "framework",
    "version": "~3.20017.0"
}
```

PlatformIO 的版本编码规则：`3.XXYYYY.0` 对应实际版本 `X.YY.YYY`，即：

```
3.20017.0  →  2.0.17
```

结论：espressif32@6.13.0 使用的是 **arduino-esp32 2.0.17**。

验证（安装后可运行）：

```bash
cat ~/.platformio/packages/framework-arduinoespressif32/package.json \
  | python3 -m json.tool | grep version
# 输出: "version": "2.0.17"
```

### 2.2 第二跳：arduino-esp32 2.0.17 → IDF 版本

arduino-esp32 会把 IDF 头文件打包进来，直接读版本：

```bash
cat ~/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/include/\
esp_common/include/esp_idf_version.h
```

关键行：

```c
#define ESP_IDF_VERSION_MAJOR   4
#define ESP_IDF_VERSION_MINOR   4
#define ESP_IDF_VERSION_PATCH   8
```

结论：默认框架包内嵌的是 **IDF 4.4.8**。

### 2.3 第三跳：IDF 版本 → lib-builder 分支

`espressif/esp32-arduino-lib-builder` 的分支命名与 IDF 大版本直接对应：

| lib-builder 分支 | 构建目标 IDF |
| ---------------- | ------------ |
| `release/v4.4`   | IDF 4.4.x    |
| `release/v5.1`   | IDF 5.1.x    |
| `release/v5.3`   | IDF 5.3.x    |
| `release/v6.0`   | IDF 5.5.x    |

目标分支：**`release/v4.4`**

```bash
git clone https://github.com/espressif/esp32-arduino-lib-builder.git \
  ~/him/esp32-arduino-lib-builder
cd ~/him/esp32-arduino-lib-builder
git checkout release/v4.4
```

---

## 第 3 章：release/v4.4 分支弃用问题与修复

### 3.1 根本原因：官方已停止维护

按第 2 章步骤切换分支后，直接运行 `./build.sh -t esp32s3` 会构建失败。

官方在 GitHub Issue
[espressif/esp32-arduino-lib-builder#292](https://github.com/espressif/esp32-arduino-lib-builder/issues/292)
明确声明：

> "We no longer support anything related to ESP-IDF 4.4.
> We only keep the branches for reference."

分支代码本身是 2023 年的状态，但被引用的外部组件仓库（esp32-camera、
esp_littlefs、tinyusb、esp-dl）的 HEAD 均已升级到 IDF 5.x 时代，导致在 IDF 4.4
下编译失败。

`update-components.sh` 默认行为是各组件仓库 `git pull`，拉到最新代码，
而最新代码已不兼容 IDF 4.4。

---

### 3.2 失败一：esp-dl idf_component.yml 版本声明冲突

**报错**：CMake 配置阶段提示组件依赖冲突

**原因**：`components/esp-dl/idf_component.yml` 里同时声明：

```yaml
dependencies:
  idf: ">=5.0"       # 只允许 IDF 5.0+
  idf: "4.4.*"       # 同时要求 4.4.x
```

两个 `idf` 字段互相矛盾，IDF 4.4 的 `idf_component_manager` 无法解析。

**修复**：删除整个 `idf_component.yml`，让 esp-dl 作为普通本地组件参与编译。

```bash
rm components/esp-dl/idf_component.yml
```

或直接在 `update-components.sh` 中 pin 到兼容提交（见 §3.6）。

---

### 3.3 失败二：esp32-camera 引用 IDF 5.x 专有组件

**报错**：

```
CMake Error: The following imported targets are referenced but missing:
    idf::esp_mm
```

**原因**：`esp32-camera` HEAD 版的 `CMakeLists.txt` 里有：

```cmake
REQUIRES ... esp_mm ...
```

`esp_mm`（内存管理器）是 IDF 5.x 新增组件，IDF 4.4 中不存在。

**修复**：pin 到 arduino-esp32 2.0.17 官方对应的提交（见 §3.6）。

---

### 3.4 失败三：esp_littlefs 要求 IDF ≥ 5.0

**报错**：

```
CMake Error: IDF version must be >= 5.0
```

**原因**：`esp_littlefs` HEAD 在 `CMakeLists.txt` 中显式检查 IDF 版本：

```cmake
if(${ESP_IDF_VERSION} VERSION_LESS "5.0")
    message(FATAL_ERROR "IDF version must be >= 5.0")
endif()
```

**修复**：pin 到 arduino-esp32 2.0.17 官方对应的提交（见 §3.6）。

---

### 3.5 失败四：TinyUSB API 签名变更

**报错**：编译 TinyUSB 时出现函数签名不匹配：

```
error: too many arguments to function 'bool dcd_edpt_xfer(...)'
```

**原因**：TinyUSB 较新版本在 `dcd_edpt_xfer` 函数末尾新增了 `bool is_isr`
参数，而 IDF 4.4 时代的 TinyUSB port 仍调用旧签名（无该参数）。

**修复**：pin 到 arduino-esp32 2.0.17 官方对应的提交（见 §3.6）。

---

### 3.6 修复策略：从官方 versions.txt 提取精确 commit

arduino-esp32 的正式发布版本会在
[`tools/sdk/versions.txt`](https://raw.githubusercontent.com/espressif/arduino-esp32/release/v2.x/tools/sdk/versions.txt)
中记录所有组件的精确 commit hash，直接使用这些值即可避免所有兼容问题。

获取（以 v2.0.17 为基准）：

```bash
curl -s https://raw.githubusercontent.com/espressif/arduino-esp32/release/v2.x/tools/sdk/versions.txt
```

从输出中提取四个相关组件的 commit：

| 组件         | Pin commit                                 |
| ------------ | ------------------------------------------ |
| esp32-camera | `2829692`                                  |
| esp-dl       | `0632d2447dd49067faabe9761d88fa292589d5d9` |
| esp_littlefs | `06050e6`                                  |
| tinyusb      | `756ad3553`                                |

---

### 3.7 update-components.sh 修改内容

将各组件的更新逻辑从 `git pull`（取最新）改为 `git reset --hard <commit>`
（pin 到精确版本）。修改后的相关片段：

```bash
# esp32-camera：pin to arduino-esp32 v2.0.17
if [ ! -d "$AR_COMPS/esp32-camera" ]; then
    git clone $CAMERA_REPO_URL "$AR_COMPS/esp32-camera" && \
    git -C "$AR_COMPS/esp32-camera" reset --hard 2829692
else
    git -C "$AR_COMPS/esp32-camera" fetch && \
    git -C "$AR_COMPS/esp32-camera" reset --hard 2829692
fi

# esp-dl：pin to last IDF 4.4 compatible commit
git -C "$AR_COMPS/esp-dl" reset --hard 0632d2447dd49067faabe9761d88fa292589d5d9

# esp_littlefs：fetch 后再 reset（浅克隆不含目标 commit，必须先 fetch）
if [ ! -d "$AR_COMPS/esp_littlefs" ]; then
    git clone $LITTLEFS_REPO_URL "$AR_COMPS/esp_littlefs" && \
    git -C "$AR_COMPS/esp_littlefs" reset --hard 06050e6 && \
    git -C "$AR_COMPS/esp_littlefs" submodule update --init --recursive
else
    git -C "$AR_COMPS/esp_littlefs" fetch && \
    git -C "$AR_COMPS/esp_littlefs" reset --hard 06050e6 && \
    git -C "$AR_COMPS/esp_littlefs" submodule update --init --recursive
fi

# tinyusb：pin to arduino-esp32 v2.0.17
if [ ! -d "$AR_COMPS/arduino_tinyusb/tinyusb" ]; then
    git clone $TINYUSB_REPO_URL "$AR_COMPS/arduino_tinyusb/tinyusb" && \
    git -C "$AR_COMPS/arduino_tinyusb/tinyusb" reset --hard 756ad3553
else
    git -C "$AR_COMPS/arduino_tinyusb/tinyusb" fetch && \
    git -C "$AR_COMPS/arduino_tinyusb/tinyusb" reset --hard 756ad3553
fi
```

> **esp-dl 特殊处理**：该仓库的浅克隆已包含目标 commit，直接 `reset --hard`
> 即可，不需要额外 `fetch`。

---

### 3.8 -s 参数：跳过组件自动更新

`build.sh` 提供 `-s` 参数，跳过 `update-components.sh` 的执行：

```bash
./build.sh -s -t esp32s3    # -s = skip component update
```

**使用场景**：

- 组件已手动 pin 到正确版本，不希望脚本再次覆盖
- `managed_components/` 目录残留旧版本缓存导致冲突，先手动
  `rm -rf managed_components` 再加 `-s` 构建
- 网络不稳定时跳过 fetch，使用缓存的组件目录

> 注意：初次克隆后第一次构建**不能加 `-s`**，因为组件目录尚不存在，
> 需要先运行一次 `update-components.sh` 正常克隆各组件。之后的重复构建
> 才适合加 `-s`。

---

## 第 4 章：PM 配置

在 `configs/defconfig.esp32s3` 文件末尾追加三行：

```bash
cat >> ~/him/esp32-arduino-lib-builder/configs/defconfig.esp32s3 << 'EOF'

# Power management
CONFIG_PM_ENABLE=y
CONFIG_FREERTOS_USE_TICKLESS_IDLE=y
CONFIG_FREERTOS_IDLE_TIME_BEFORE_SLEEP=3
EOF
```

各配置项说明：

| 配置项                                     | 作用                                                                        |
| ------------------------------------------ | --------------------------------------------------------------------------- |
| `CONFIG_PM_ENABLE=y`                       | 解锁 `esp_pm_configure()` API，使其真正生效而非返回 `ESP_ERR_NOT_SUPPORTED` |
| `CONFIG_FREERTOS_USE_TICKLESS_IDLE=y`      | FreeRTOS idle 时允许关闭 Systick，配合 PM 实现动态频率调节                  |
| `CONFIG_FREERTOS_IDLE_TIME_BEFORE_SLEEP=3` | 连续 idle 超过 3 个 tick 才触发降频（避免频繁切换开销）                     |

配置完成后执行构建：

```bash
cd ~/him/esp32-arduino-lib-builder
./build.sh -t esp32s3 2>&1 | tee ./.vscode/logs/build-$(date +%Y%m%d-%H%M%S).log
```

构建成功的标志是日志末尾出现 `[1899/1899]`（esp32s3 目标的总编译单元数）。

---

## 第 5 章：打包

IDF 4.4 是单包架构：`tools/sdk/esp32s3/`（预编译 IDF 库）内嵌在
`framework-arduinoespressif32` 包内，因此需要以 arduino-esp32 2.0.17 为底包，
替换其中的 sdk 目录，整体重新打包。

### 5.1 底包：克隆 arduino-esp32 2.0.17

不直接使用 `~/.platformio/packages/framework-arduinoespressif32/`，原因：
该目录当前已被 PlatformIO 管理，改动后会导致缓存状态混乱。
独立克隆一份确保干净：

```bash
git clone \
  --branch 2.0.17 \
  --depth 1 \
  https://github.com/espressif/arduino-esp32.git \
  ~/him/arduino-esp32-2.0.17
```

### 5.2 替换 sdk 目录

```bash
# 删除原有 IDF 4.4 默认静态库
rm -rf ~/him/arduino-esp32-2.0.17/tools/sdk/esp32s3

# 替换为自定义编译版本（含 CONFIG_PM_ENABLE=y）
cp -r ~/him/esp32-arduino-lib-builder/out/tools/sdk/esp32s3 \
      ~/him/arduino-esp32-2.0.17/tools/sdk/

# 同步 lib-builder 生成的 platformio build 脚本
# （lib-builder 会修改这个脚本以匹配新的链接器参数）
cp ~/him/esp32-arduino-lib-builder/out/tools/platformio-build-esp32s3.py \
   ~/him/arduino-esp32-2.0.17/tools/
```

### 5.3 打包为 tarball

```bash
cd ~/him
tar cJf framework-arduinoespressif32-pm-idf44.tar.xz arduino-esp32-2.0.17/
# 打包完成约 2.2GB（xz 压缩），耗时视 CPU 而定（约 10-30 分钟）
```

打包命令以 `arduino-esp32-2.0.17/` 作为 tarball 中的顶层目录。
PlatformIO 的包管理器在安装 tarball 时会**自动剥离唯一的顶层目录**（标准行为，
与 GitHub release 下载包一致），安装后路径为：

```
~/.platformio/packages/framework-arduinoespressif32/
├── cores/
├── libraries/
├── tools/
│   ├── sdk/
│   │   └── esp32s3/    ← 替换后的自定义 IDF 库
│   └── platformio-build-esp32s3.py
└── variants/
```

---

## 第 6 章：集成与验证

### 6.1 platformio.ini 修改

在 `variants/esp32s3/eink-ra01s-gp02/platformio.ini` 末尾追加两行，
其余保持不变：

```ini
platform = platformio/espressif32@6.13.0
platform_packages =
    framework-arduinoespressif32 @ file:///home/sainstroe/him/framework-arduinoespressif32-pm-idf44.tar.xz
```

PlatformIO 首次构建时会将 tarball 解压到
`~/.platformio/packages/framework-arduinoespressif32/`，覆盖已有缓存。

### 6.2 验证层一：sdkconfig（框架配置层）

确认 tarball 被正确解压，且 PM 配置存在于已安装框架的 sdkconfig：

```bash
grep -E "CONFIG_PM_ENABLE|CONFIG_FREERTOS_USE_TICKLESS" \
  ~/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/sdkconfig
```

预期输出：

```
CONFIG_PM_ENABLE=y
CONFIG_FREERTOS_USE_TICKLESS_IDLE=y
```

### 6.3 验证层二：map 文件（链接器层）

Meshtastic 编译成功后，`.pio/build/eink-ra01s-gp02/` 下会生成 `.map` 文件。
在其中查找 PM API 符号是否具有真实链接地址（非零地址）：

```bash
grep -E "esp_pm_configure|esp_pm_lock_create|esp_pm_lock_acquire" \
  .pio/build/eink-ra01s-gp02/*.map | grep -v "0x0000000000000000"
```

预期输出（地址非零即为真实实现，非 stub）：

```
0x0000000040375990   esp_pm_lock_acquire
0x0000000042043948   esp_pm_lock_create
```

同时确认这些符号来自我们的自定义包路径：

```
~/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/lib/libesp_pm.a
```

若三项均通过：**自定义 framework 包已完整集成，PM_ENABLE 生效**。

---

## 附录：为什么需要 CONFIG_PM_ENABLE

`src/platform/esp32/main-esp32.cpp` 第 161 行存在一行被注释掉的调用：

```cpp
// enableModemSleep();
```

`enableModemSleep()`（定义于 `src/sleep.cpp`）通过调用 `esp_pm_configure()` 启用
DFS（动态频率调节），CPU 在 FreeRTOS idle 时自动降至 20MHz，可将平均电流从
~100mA 降至 ~36mA（实测，含 BLE 开启状态，来自社区 Issue
[#8876](https://github.com/meshtastic/firmware/issues/8876)）。

该行于 2020 年 6 月（commit `d8db4449b`，版本 0.7.6）由创始人 geeksville
**在引入的同时就注释掉**，理由记录在函数注释中：

> "supposedly according to arduino-esp32#475 this is already done in arduino"

即当时认为 Arduino 框架已自动开启 modem sleep，显式调用是多余的。但实际上：
**默认的 arduino-esp32 2.0.x 未启用 `CONFIG_PM_ENABLE`，`esp_pm_configure()` 在
该环境下是无操作的空函数**，注释的存在掩盖了这个根本问题。

构建本文记录的自定义框架包后，该注释可安全解除，`esp_pm_configure()` 将真正生效。
解除前请参考 Meshtastic 官方 PR
[#7121](https://github.com/meshtastic/firmware/pull/7121)（动态 light sleep
完整实现，当前仍为 Open 状态）评估是否同步实施更完整的 PM 策略。
