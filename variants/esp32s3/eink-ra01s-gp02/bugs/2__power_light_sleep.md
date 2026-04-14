# Auto Light-Sleep 功耗优化分析记录

> 日期：2026-04-13
> 固件：Meshtastic v2.7.22.2c633b6，target: `eink-ra01s-gp02`
> 硬件：ESP32-S3-WROOM-1U-N16R8（16MB Flash QIO，8MB PSRAM OPI），无 32.768kHz 外部晶振

---

## 1. 概述

### 现象

上电待机电流约 100mA，参考 ESP-IDF 官方 power_save 例程（IDF v4.4，同款硬件）仅约 14mA。目标是缩小差距。

### 测量结果

| 阶段                        | 功耗         | 说明                                                |
| --------------------------- | ------------ | --------------------------------------------------- |
| 基准（未优化）              | ~100mA       | 原始 Meshtastic 固件                                |
| 仅修改 IDF 配置层           | ~50mA        | BT modem sleep + MAC/BB PD + UNICORE 生效，DFS 降频 |
| IDF 配置 + auto light-sleep | ~6mA（异常） | light_sleep 开启但时序错误，设备卡死不运行          |
| 完整修复后（预期）          | ~15mA        | light_sleep 时序正确 + v4.4 UART bug 修复           |

### 涉及三个层次

- **IDF 配置层**：`defconfig.esp32s3` — 静态库编译时选项，决定 BT modem sleep / MAC BB PD / tickless idle 等功能是否可用
- **固件代码层**：`sleep.cpp` / `main-esp32.cpp` — light-sleep 启用时序、UART GPIO 隔离修复
- **构建部署层**：lib-builder 重编译 → 打包 → PlatformIO 缓存清理

以下三章各自覆盖一个层次，每章独立闭环。

---

## 2. IDF 配置层（defconfig.esp32s3）

文件路径：`/home/sainstroe/him/esp32-arduino-lib-builder/configs/defconfig.esp32s3`

这是 esp32-arduino-lib-builder 的 ESP32-S3 编译配置，决定**静态库中哪些功能被编译进去**。
Meshtastic 官方包的默认配置缺少以下项，导致 power_save 例程能跑、Meshtastic 不能。

### 2.1 缺失项及影响

| 缺失配置项                                       | 原值         | 修改后 | 影响                                                                                                  |
| ------------------------------------------------ | ------------ | ------ | ----------------------------------------------------------------------------------------------------- |
| `CONFIG_FREERTOS_UNICORE`                        | 未设（双核） | `y`    | 双核时 CPU1 的 `rtos1 CPU_FREQ_MAX` lock 常驻，阻止 DFS 降频。日志证据：`rtos1 CPU_FREQ_MAX Active=1` |
| `CONFIG_BT_CTRL_MODEM_SLEEP`                     | `n`          | `y`    | BLE controller 不休眠，射频持续活跃（~30-50mA）                                                       |
| `CONFIG_BT_CTRL_MODEM_SLEEP_MODE_1`              | 未设         | `y`    | 选 Mode 1：BLE 广播间隙期间射频断电                                                                   |
| `CONFIG_BT_CTRL_LPCLK_SEL_MAIN_XTAL`             | 未设         | `y`    | 用 40MHz 主晶振作 BLE LP 时钟（本板无 32.768kHz 外部晶振）                                            |
| `CONFIG_BT_CTRL_MAIN_XTAL_PU_DURING_LIGHT_SLEEP` | 未设         | `y`    | light sleep 期间保持 XTAL 通电，BLE 计时不中断                                                        |
| `CONFIG_ESP_PHY_MAC_BB_PD`                       | `n`          | `y`    | PHY 空闲时不下电 MAC/baseband（~10-15mA）                                                             |
| `CONFIG_PM_SLP_IRAM_OPT`                         | `n`          | `y`    | light sleep 入/出代码移至 IRAM，减少 flash 唤醒延迟                                                   |
| `CONFIG_PM_RTOS_IDLE_OPT`                        | `n`          | `y`    | idle 判断代码移至 IRAM                                                                                |

> `CONFIG_PM_SLP_DISABLE_GPIO` **不加**：v4.4 下该选项 + UART 驱动 bug 会导致控制台卡死（见 §3.2）。

### 2.2 Kconfig 依赖链

修改前需确认各项的依赖都已满足：

```
FREERTOS_USE_TICKLESS_IDLE=y          ← 前置（PM_ENABLE 推导出，defconfig 已有）
  └─ BT_CTRL_MAIN_XTAL_PU_DURING_LIGHT_SLEEP  depends on LPCLK_SEL_MAIN_XTAL && TICKLESS_IDLE
  └─ ESP_PHY_MAC_BB_PD                         depends on IDF_TARGET_ESP32S3 && TICKLESS_IDLE
  └─ PM_SLP_IRAM_OPT                           depends on TICKLESS_IDLE
  └─ PM_RTOS_IDLE_OPT                          depends on TICKLESS_IDLE

BT_CTRL_HCI_MODE_UART_H4=n (VHCI 模式)
  └─ BT_CTRL_MODEM_SLEEP                       depends on !HCI_MODE_UART_H4  ✅ 当前 VHCI
```

依赖链来源：`/home/sainstroe/him/esp-idf-v4.4/components/bt/controller/esp32c3/Kconfig.in`（ESP32-S3 直接 source 此文件）和 `components/esp_phy/Kconfig`。

### 2.3 添加到 defconfig.esp32s3 的内容

```kconfig
# Single core — eliminates rtos1 CPU_FREQ_MAX lock
CONFIG_FREERTOS_UNICORE=y

# BT controller modem sleep (VHCI mode, main XTAL as LP clock)
CONFIG_BT_CTRL_MODEM_SLEEP=y
CONFIG_BT_CTRL_MODEM_SLEEP_MODE_1=y
CONFIG_BT_CTRL_LPCLK_SEL_MAIN_XTAL=y
CONFIG_BT_CTRL_MAIN_XTAL_PU_DURING_LIGHT_SLEEP=y

# Power down MAC/baseband when PHY idle
CONFIG_ESP_PHY_MAC_BB_PD=y

# Auto light-sleep IRAM optimization
CONFIG_PM_SLP_IRAM_OPT=y
CONFIG_PM_RTOS_IDLE_OPT=y
```

### 2.4 验证方法

lib-builder 输出的 `out/tools/sdk/esp32s3/sdkconfig` 中确认以下行存在且值正确：

```bash
grep -E "UNICORE|BT_CTRL_MODEM|BT_CTRL_LPCLK|XTAL_PU|PHY_MAC_BB|PM_SLP_IRAM|PM_RTOS_IDLE" \
  /home/sainstroe/him/esp32-arduino-lib-builder/out/tools/sdk/esp32s3/sdkconfig
```

安装到 PlatformIO 后同样验证 `sdkconfig.h`：

```bash
grep -E "UNICORE|BT_CTRL_MODEM|BT_CTRL_SLEEP_MODE_EFF|SLEEP_CLOCK_EFF|PHY_MAC_BB|PM_SLP_IRAM|PM_RTOS_IDLE" \
  ~/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/qio_opi/include/sdkconfig.h
```

---

## 3. 固件代码层（sleep.cpp / main-esp32.cpp）

IDF 配置层只决定功能是否**可用**，代码层决定功能**何时启用**。这里有两个独立 bug。

### 3.1 Light-sleep 启用时序问题

#### 现象

将 `esp32_config.light_sleep_enable = true` 后烧录，功耗降至 6mA，但设备卡死：
串口在 `Sleep request result 0` 后截断，BLE 不初始化，不响应任何操作。

日志证据（`build-20260413-155139.log`）：

```
[34mSleep request resu    ← 截断，后续无任何输出
```

#### 根因

`enableModemSleep()` 在 `main-esp32.cpp:161` 调用，位于初始化最开始阶段（preferences 加载后、NodeDB/Mesh/BLE **全部之前**）。

调用时序：

```
T=0s  enableModemSleep(light_sleep=true)  ← BLE 尚未初始化，无周期性唤醒源
T+3ms FreeRTOS 所有任务 idle → 满足 tickless idle 条件 → 进入 light sleep
      无 BLE advertising 唤醒 → 永久睡死
```

BLE 要到 `PowerFSM_setup()`（`main.cpp:1012`）之后状态机转换到 ON 态（约 3s 后）才由 `setBluetoothEnable(true)` 触发初始化，远晚于 `enableModemSleep()`。

参考对象（power_save 例程）的正确顺序：

```
nimble_port_freertos_init()   → BLE Host Task 启动
vTaskDelay(500ms)             → 等待 bleprph_on_sync，advertising 已稳定
esp_pm_configure(light_sleep=true)  ← 此时才启用
```

#### 修复方案

将 light-sleep 的启用拆分为两步：

1. `enableModemSleep()`（保持原调用位置）：只配置 DFS（`light_sleep_enable = false`），降低 CPU 频率，不会导致卡死。

2. 新增 `enableLightSleep()`：在 `setBluetoothEnable()` 内 `nimbleBluetooth->setup()` 完成后调用，此时 BLE advertising 已启动，有周期性唤醒源。

`enableLightSleep()` 内用 `static bool enabled` 保证多次调用（PowerFSM 多个状态都会触发 `setBluetoothEnable(true)`）只执行一次。

修改文件：

- `src/sleep.cpp`：`enableModemSleep()` 改回 `light_sleep_enable = false`；新增 `enableLightSleep()`
- `src/sleep.h`：声明 `enableLightSleep()`
- `src/platform/esp32/main-esp32.cpp`：在 `nimbleBluetooth->setup()` 后调用 `enableLightSleep()`

---

### 3.2 IDF v4.4 UART GPIO 隔离 bug

#### 现象

即使时序正确，开启 auto light-sleep 后串口控制台可能出现乱码或无响应。
若同时启用 `CONFIG_PM_SLP_DISABLE_GPIO`，则 UART 控制台**完全**无法交互。

#### 根因

IDF v4.4 的 `uart_set_pin()` 在设置 TX/RX 引脚时，未调用 `gpio_sleep_sel_dis()` 将其排除出 sleep 隔离列表。
结果：light sleep 期间 GPIO sleep 机制隔离所有未显式豁免的引脚，UART0 TX/RX（GPIO 43/44）也被隔离 → 串口静音。

该问题在 IDF v5.0+ 中已在 `uart_set_pin()` 内部修复。

#### 修复方案

在 `enableLightSleep()` 内，针对 `arduino-esp32 < 3.0.0`（即 IDF v4.4 框架）且 ESP32-S3 目标，手动豁免 UART0 引脚：

```cpp
// IDF v4.4: UART driver bug — exclude UART0 TX/RX from GPIO sleep isolation
#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3, 0, 0) && CONFIG_IDF_TARGET_ESP32S3
    gpio_sleep_sel_dis(GPIO_NUM_43); // UART0 TX
    gpio_sleep_sel_dis(GPIO_NUM_44); // UART0 RX
#endif
```

`gpio_sleep_sel_dis()` 声明在 `driver/gpio.h`，`sleep.cpp` 已添加该 include。
GPIO 43/44 是 ESP32-S3-WROOM 模块的默认 UART0 引脚（固定，不可更改）。

> **为何不加 `CONFIG_PM_SLP_DISABLE_GPIO`**：该选项会全局禁用所有 GPIO sleep 隔离，覆盖范围过宽，且与上述手动豁免方案冲突，在 v4.4 下反而加剧问题。

---

## 4. 构建部署

### 4.1 重新编译 IDF 静态库

```bash
cd /home/sainstroe/him/esp32-arduino-lib-builder
./build.sh -s -t esp32s3
```

完成后验证输出 sdkconfig（参见 §2.4）。

### 4.2 打包 framework tarball

```bash
cd /home/sainstroe/him/esp32-arduino-lib-builder
tar -cJf /home/sainstroe/him/framework-arduinoespressif32-pm-idf44.tar.xz \
    --transform 's|^out|package|' out/
```

### 4.3 清理 PlatformIO 缓存（必须）

PlatformIO 对 `file://` 本地 TAR 包**不检查内容变化**，仅凭文件名判断是否重新解压。
同名 tarball 更新后，必须手动删除旧缓存，否则编译使用的仍是旧库。

**正常安装后的目录状态：**

```bash
ls ~/.platformio/packages/ | grep framework-arduinoespressif32
# 正常只有两项：
framework-arduinoespressif32          ← file:// tarball 安装的主目录
framework-arduinoespressif32-libs     ← 工具链库，不要删
```

**如果出现 `@src-HASH` 后缀的目录：**

```bash
framework-arduinoespressif32@src-abc123def456...   ← 冲突时被搬迁的旧包
```

这是 PlatformIO 包管理器的冲突解决机制：当新包安装时发现同名包已存在，
若版本发生变化，会将**旧包以 `@src-MD5(旧URI)` 的方式移开**，新包占据主目录。
`@src-*` 目录是被搬迁的旧版本残留，可安全删除。

**清理命令：**

```bash
# 删除旧版本残留（如有）
rm -rf ~/.platformio/packages/framework-arduinoespressif32@src-*

# 删除当前主目录（触发 PlatformIO 重新从 tarball 解压安装）
rm -rf ~/.platformio/packages/framework-arduinoespressif32
```

> **每次更新 tarball 后都需执行此步骤**，否则修改的 defconfig 不会生效。

### 4.4 重新编译 Meshtastic 固件

```bash
cd /home/sainstroe/him/firmware
pio run -e eink-ra01s-gp02
```

---

## 5. 测试记录

### 5.1 各阶段实测功耗

| 阶段                                   | 日志文件                    | 实测功耗      | 关键日志行                                         |
| -------------------------------------- | --------------------------- | ------------- | -------------------------------------------------- |
| 基准（官方框架包）                     | —                           | ~100mA        | `rtos1 CPU_FREQ_MAX Active=1`                      |
| 自定义框架包（IDF 配置修复，DFS only） | `build-20260413-154907.log` | ~50mA         | `Lock stats: rtos0 CPU_FREQ_MAX 0 1`（已无 rtos1） |
| light_sleep=true（时序错误）           | `build-20260413-155139.log` | ~6mA（卡死）  | `Sleep request resu`（截断，无后续输出）           |
| 完整修复后                             | —                           | ~15mA（预期） | 待烧录含 `enableLightSleep()` 的固件验证           |

日志存放目录：`.pio/logs/eink-ra01s-gp02/`

### 5.2 关键诊断日志片段

**第一阶段诊断**（`esp_pm_dump_locks` 输出，揭示双核 lock 问题）：

```
Sleep request result 0
Lock stats:
Name            Type            Arg    Active
rtos0           CPU_FREQ_MAX    0      1
rtos1           CPU_FREQ_MAX    0      1    ← CPU1 idle lock，阻止 DFS
```

**IDF 配置修复后**（`build-20260413-154907.log` 第 44-46 行）：

```
Lock stats:
Name            Type            Arg    Active
rtos0           CPU_FREQ_MAX    0      1    ← 仅剩 CPU0，UNICORE 生效
```

### 5.3 参考工程

- 官方 power_save 例程：`/home/sainstroe/him/examples/power_save_v448`
- 移植版（含 console + UART 修复）：`/home/sainstroe/him/esp_idf_pro/library_components/IoT/power_save`
- 移植版完整 sdkconfig：`/home/sainstroe/him/esp_idf_pro/sdkconfig`
- power_save 运行日志：`/home/sainstroe/him/esp_idf_pro/.vscode/logs/v448 2026-04-13 16:54:10.log`
