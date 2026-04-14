# 自定义 ESP32 SDK 构建笔记

> 目标：为 eink-ra01s-gp02（ESP32-S3）构建含 `CONFIG_PM_ENABLE=y` 的 IDF 4.4 预编译库，
> 以最小改动集成进 Meshtastic 工程。

---

## 第 1 章：PlatformIO / arduino-esp32 / IDF 的框架层级

### 1.1 三层结构

```
┌─────────────────────────────────────────────────────────────┐
│  Layer 3: PlatformIO platform                               │
│   platformio/espressif32@6.13.0                             │
│   描述构建工具链、框架包列表、上传工具。                     │
│   本身不含任何 ESP32 源码或 IDF 代码。                      │
└───────────────────────┬─────────────────────────────────────┘
                        │ 引用
┌───────────────────────▼─────────────────────────────────────┐
│  Layer 2: Framework — framework-arduinoespressif32          │
│   arduino-esp32 source (cores/, libraries/, variants/)     │
│   提供 Arduino API 实现（Serial, WiFi, BLE...）              │
│   这一层是 C++ 源码，由 PlatformIO 在你的工程里编译。        │
└───────────────────────┬─────────────────────────────────────┘
                        │ 链接
┌───────────────────────▼─────────────────────────────────────┐
│  Layer 1: IDF Prebuilt Libs                                 │
│   已编译的 ESP-IDF 静态库（libbt.a, liblwip.a, ...）         │
│   由 espressif/esp32-arduino-lib-builder 构建产出。          │
│   框架层的 Arduino API 最终调用这些库的函数。                │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 两个时代的打包架构差异（关键）

**arduino-esp32 2.x 时代（IDF 4.4）— 单包架构**

IDF 预编译库内嵌在 `framework-arduinoespressif32` 包内部：

```
framework-arduinoespressif32/
├── cores/
├── libraries/
├── variants/
└── tools/
    └── sdk/
        └── esp32s3/          ← IDF 4.4 預編譯庫放在这里
            ├── include/
            ├── lib/
            ├── ld/
            └── sdkconfig
```

替换 libs = 替换整个 framework 包（含 Arduino 源码 + IDF libs）。

---

**arduino-esp32 3.x 时代（IDF 5.x）— 双包架构**

IDF 预编译库独立拆分为第二个包：

```
framework-arduinoespressif32/          ← 仅含 Arduino 源码
├── cores/
├── libraries/
└── variants/

framework-arduinoespressif32-libs/     ← 仅含 IDF 預編譯库
└── esp32s3/
    ├── include/
    ├── qio_opi/
    │   ├── lib/
    │   └── include/sdkconfig.h
    ├── ld/
    └── pioarduino-build.py            ← 生成的构建脚本
```

替换 libs = 只替换 `-libs` 包，arduino 源码包不动。

---

### 1.3 PlatformIO 如何选择用哪个包

`platformio.ini` 中 `platform_packages` 可以覆盖平台默认的包版本：

```ini
; 使用官方平台但替换 framework 包（IDF 4.4 路线）
platform = platformio/espressif32@6.13.0
platform_packages =
    framework-arduinoespressif32 @ file:///path/to/custom-framework.tar.gz
```

```ini
; 使用 pioarduino 平台并替换 libs 包（IDF 5.x 路线，不适用本工程）
platform = https://github.com/pioarduino/platform-espressif32.git#develop
platform_packages =
    framework-arduinoespressif32 @ https://...arduino-esp32 3.x...
    framework-arduinoespressif32-libs @ file:///path/to/libs.tar.xz
```

**结论：IDF 4.4 路线只需替换一个包；IDF 5.x 路线需要同时替换两个包。**

---

## 第 2 章：espressif 相关仓库关系

### 2.1 仓库关系图

```
espressif/esp-idf                          ← ESP-IDF 核心
https://github.com/espressif/esp-idf
  版本：v4.4.x（本工程目标）
  说明：IDF 本体，含 FreeRTOS、lwIP、BT、WiFi 等组件源码

        │ lib-builder 用 esp-idf 编译生成静态库
        ▼

espressif/esp32-arduino-lib-builder        ← 构建 IDF 静态库的脚本集
https://github.com/espressif/esp32-arduino-lib-builder
  分支：release/v4.4（对应 IDF 4.4 + arduino-esp32 2.x）
        release/v6.0（对应 IDF 5.5 + arduino-esp32 3.x，当前 HEAD）
  说明：运行 build.sh，拉取 esp-idf + arduino-esp32，
        编译出 tools/sdk/<target>/ 下的静态库

        │ 输出 out/tools/sdk/esp32s3/
        ▼

espressif/arduino-esp32                    ← Arduino for ESP32 源码
https://github.com/espressif/arduino-esp32
  版本关联：2.x ↔ IDF 4.4 / 3.x ↔ IDF 5.x
  说明：提供 cores/esp32、libraries/ 的 Arduino 实现，
        最终要与上面的静态库打包在一起

  注意：这个仓库不是 arduino 官方的 IDE core，
        也不是 pioarduino 社区的 fork（见下）

        │ PlatformIO 平台将两者打包
        ▼

platformio/platform-espressif32            ← PlatformIO 官方 ESP32 平台
https://github.com/platformio/platform-espressif32
  版本：6.13.0（Meshtastic 使用）
  内含：arduino-esp32 2.0.17 + IDF 4.4 预编译库（单包）
  工具链：GCC 8.4.0

pioarduino/platform-espressif32            ← 社区 fork（非官方）
https://github.com/pioarduino/platform-espressif32
  说明：Jason2866 维护，跟进最新 arduino-esp32 3.x + IDF 5.x，
        比官方平台更新更快，双包架构
  工具链：GCC 14.2.0
```

### 2.2 容易混淆的地方

| 仓库                                  | 是什么                    | 不是什么           |
| ------------------------------------- | ------------------------- | ------------------ |
| `espressif/esp-idf`                   | IDF 核心全量源码          | Arduino 库         |
| `espressif/arduino-esp32`             | ESP32 的 Arduino 实现     | Arduino IDE 官方   |
| `espressif/esp32-arduino-lib-builder` | 编译 IDF 静态库的脚本     | 固件工程           |
| `platformio/platform-espressif32`     | PlatformIO 平台描述文件   | 编译器或 IDE       |
| `pioarduino/platform-espressif32`     | 社区维护的更新版 PIO 平台 | espressif 官方产品 |

---

## 第 3 章：Meshtastic 的版本锁定与影响

### 3.1 Meshtastic 的官方基线

`variants/esp32/esp32-common.ini`（所有 ESP32/S2/S3 板的基类）：

```ini
platform = platformio/espressif32@6.13.0
```

对应关系：

| 组件                | 版本                     |
| ------------------- | ------------------------ |
| PlatformIO platform | espressif32@6.13.0       |
| arduino-esp32       | 2.0.17（单包内嵌）       |
| ESP-IDF             | 4.4.x                    |
| GCC 工具链          | 8.4.0                    |
| NimBLE-Arduino(BLE) | 1.4.3（为 IDF 4.4 设计） |

### 3.2 为什么 Meshtastic 不升级

- 大量外部库（NimBLE-Arduino、libpax、esp32_https_server）固定在 IDF 4.4 兼容版本
- IDF 4.4 → 5.x 有多处 Breaking API 变更（WDT、LEDC、WiFi 类型等）
- 升级需要同步验证所有硬件板，工作量大

### 3.3 自己写空工程 vs 使用 Meshtastic 工程

**自己写的空工程（不依赖任何历史库）：**

- 选 pioarduino + arduino-esp32 3.x + IDF 5.x 完全没有兼容问题
- IDF 5.x 的 PM、LEDC、WiFi API 新写法直接用即可
- 本章的所有注意事项全部不适用

**Meshtastic 工程：**

- 代码库假定 IDF 4.4 环境
- 依赖的第三方库版本已固定（NimBLE 1.4.3 / libpax 等）
- 升级 IDF 版本会引发连锁不兼容，需要逐个修复
- **正确做法：保持 IDF 4.4，只替换预编译 libs（加入 `CONFIG_PM_ENABLE=y`）**

---

## 第 4 章：为 IDF 4.4 构建自定义 libs（完整操作步骤）

### 4.1 前置条件

```bash
# Ubuntu/Debian 依赖
sudo apt-get install git wget curl libssl-dev libncurses-dev \
  flex bison gperf python3 python3-pip python3-setuptools \
  python3-serial python3-click python3-cryptography \
  python3-future python3-pyparsing python3-pyelftools \
  cmake ninja-build ccache jq

pip3 install --upgrade pip
```

### 4.2 克隆 lib-builder 并切换到 IDF 4.4 分支

```bash
git clone https://github.com/espressif/esp32-arduino-lib-builder.git
cd esp32-arduino-lib-builder
git checkout release/v4.4
```

> **注意**：当前 `~/him/esp32-arduino-lib-builder` 在 `release/v6.0` 分支（IDF 5.5）。
> IDF 4.4 必须在 `release/v4.4` 分支下构建。

### 4.3 添加 PM 配置

编辑 `configs/defconfig.esp32s3`，在文件末尾追加：

```bash
cat >> configs/defconfig.esp32s3 << 'EOF'

# Power management
CONFIG_PM_ENABLE=y
CONFIG_FREERTOS_USE_TICKLESS_IDLE=y
CONFIG_FREERTOS_IDLE_TIME_BEFORE_SLEEP=3
EOF
```

### 4.4 执行构建

```bash
# 只构建 esp32s3 目标（加速）
./build.sh -t esp32s3

# 构建过程耗时约 30-60 分钟，视机器性能而定
# 构建完成后输出在 out/ 目录
```

### 4.5 IDF 4.4 的输出结构

```
out/
├── platform.txt
├── package_esp32_index.template.json
└── tools/
    └── sdk/
        └── esp32s3/         ← 这就是需要替换进 framework 包的 libs
            ├── include/
            ├── lib/
            ├── ld/
            ├── sdkconfig
            └── platformio-build-esp32s3.py
```

### 4.6 重打包 framework 包

IDF 4.4 是单包架构，需要将新的 `tools/sdk/esp32s3/` 替换进官方 framework 包，再重新打 tarball。

```bash
# Step 1: 获取官方 framework-arduinoespressif32 包（arduino-esp32 2.0.17）
# 从已安装的 PlatformIO 包目录复制一份，或直接下载
cp -r ~/.platformio/packages/framework-arduinoespressif32 ~/custom-framework/

# Step 2: 用新构建的 sdk 替换旧的
rm -rf ~/custom-framework/tools/sdk/esp32s3
cp -r out/tools/sdk/esp32s3 ~/custom-framework/tools/sdk/

# Step 3: 同步 platformio build 脚本（lib-builder 会更新它）
cp out/tools/platformio-build-esp32s3.py ~/custom-framework/tools/

# Step 4: 打包为 tarball
cd ~/custom-framework
tar cJf ~/him/framework-arduinoespressif32-pm-idf44.tar.xz .
```

> **注意**：如果 `~/.platformio/packages/framework-arduinoespressif32` 当前安装的是
> 3.3.7（IDF 5.5 版本），需要单独下载 2.0.17 版本的包。
> 下载地址见 `~/.platformio/platforms/espressif32@6.13.0/platform.json` 中的 URL。

### 4.7 配置 platformio.ini

在 `variants/esp32s3/eink-ra01s-gp02/platformio.ini` 末尾：

```ini
; 保持官方平台（IDF 4.4 / arduino-esp32 2.x 兼容环境）
; 只替换 framework 包，注入自定义 libs（含 CONFIG_PM_ENABLE=y）
platform = platformio/espressif32@6.13.0
platform_packages =
    framework-arduinoespressif32 @ file:///home/sainstroe/him/framework-arduinoespressif32-pm-idf44.tar.xz
```

这样 Meshtastic 的所有源码、第三方库、工具链均保持不变，只有 IDF 静态库被替换。

### 4.8 验证构建结果

构建成功后，确认 PM 配置已进入编译结果：

```bash
# 查找 sdkconfig 中的 PM 配置
grep "CONFIG_PM_ENABLE\|CONFIG_FREERTOS_USE_TICKLESS" \
  ~/custom-framework/tools/sdk/esp32s3/sdkconfig
# 预期输出：
# CONFIG_PM_ENABLE=y
# CONFIG_FREERTOS_USE_TICKLESS_IDLE=y
```

---

## 附录

### lib-builder 分支对应关系

| lib-builder 分支 | ESP-IDF 版本 | arduino-esp32 版本 | PlatformIO 平台                 |
| ---------------- | ------------ | ------------------ | ------------------------------- |
| `release/v4.4`   | IDF 4.4.x    | 2.0.x              | platformio/espressif32@6.x      |
| `release/v5.1`   | IDF 5.1.x    | 3.0.x              | —                               |
| `release/v5.3`   | IDF 5.3.x    | 3.1.x              | —                               |
| `release/v5.4`   | IDF 5.4.x    | 3.2.x              | —                               |
| `release/v6.0`   | IDF 5.5.x    | 3.3.x              | pioarduino/platform-espressif32 |

### 快速参考命令

```bash
# 切换 lib-builder 到 IDF 4.4 分支
cd ~/him/esp32-arduino-lib-builder
git checkout release/v4.4

# 检查当前分支
git branch --show-current

# 单目标构建（快）
./build.sh -t esp32s3

# 查看构建输出
ls out/tools/sdk/esp32s3/
```
