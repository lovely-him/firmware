# 烧录后重启问题排查记录

> 日期：2026-04-13
> 固件：Meshtastic v2.7.22.2c633b6，target: `eink-ra01s-gp02`

---

## 1. 背景与硬件状态

| 项目         | 值                                                       |
| ------------ | -------------------------------------------------------- |
| MCU          | ESP32-S3-WROOM-1U-N16R8（16MB Flash QIO，8MB PSRAM OPI） |
| 无线电       | SX1262（RA-01S），所有引脚**未接线**                     |
| variant 配置 | `HAS_RADIO=0`，`HAS_GPS=0`，`HAS_SCREEN=0`               |
| 构建系统     | PlatformIO espressif32@6.13.0，arduino-esp32 v2.0.17     |

---

## 2. 现象

烧录后设备有时重启、有时正常启动（**间歇性**）。

旧日志（`build-20260413-123125.log`）关键串口输出：

```
12:34:24.780 > INFO  | ??:??:?? 0  Start meshradio init
12:34:35.752 > INFO  | ??:??:?? 11 SX126x init result -2
12:34:35.752 > WARN  | ??:??:?? 11 No SX1262 radio
12:34:35.752 > ERROR | ??:??:?? 11 NOTE! Record critical error 3 at src/main.cpp:1001
```

`meshradio init` **阻塞约 11 秒**后触发 `critical error 3`（`NO_RADIO`），TWDT 在此期间有时触发重启（间歇性）。

---

## 3. 根因一：`USE_SX1262` 绕过了 `HAS_RADIO=0`

**结论**：`initLoRa()` 以预处理器宏 `#if defined(USE_SX1262)` 作为分支条件，而非运行时的 `HAS_RADIO`。`HAS_RADIO=0` 只影响驱动层，无法阻止 RadioLib 实例化 SX1262Interface；后者用全 -1 的引脚尝试 SPI 通信，产生约 **11 秒的超时阻塞**。

**关键代码位置**：

| 文件                                             | 行号 | 内容                                                                                       |
| ------------------------------------------------ | ---- | ------------------------------------------------------------------------------------------ |
| `src/mesh/RadioInterface.cpp`                    | 232  | `#if defined(USE_SX1262) && !defined(ARCH_PORTDUINO)` — 此处决定是否实例化 SX1262Interface |
| `variants/esp32s3/eink-ra01s-gp02/lora_sx1262.h` | 12   | 原 `#define USE_SX1262`，已注释                                                            |
| `src/main.cpp`                                   | 1001 | `RECORD_CRITICALERROR(meshtastic_CriticalErrorCode_NO_RADIO)`                              |

**修复**：注释 `lora_sx1262.h:12` 的 `#define USE_SX1262`。

> `#include "lora_sx1262.h"` 保留不删除：该文件将所有 `LORA_*` 引脚设为 -1，阻止 `architecture.h` 回退到 ESP32 经典默认值（GPIO 5/18/19/27），否则会踩踏 OPI PSRAM 引脚，触发 TG1WDT。

**修复后日志对比**（`build-20260413-125005.log`）：

```
12:54:52.158 > ERROR | ??:??:?? 0  NOTE! Record critical error 3 at src/main.cpp:1001
12:54:52.158 > INFO  | ??:??:?? 0  PowerFSM init, USB power=1
```

`critical error 3` 在 uptime=**0** 立即触发，无 11 秒阻塞；`SX126x init result -2` 不再出现。

---

## 4. 根因二：BLE 初始化期间欠压（`BROWNOUT_RST`）

**结论**：修复根因一后，启动流程顺利到达 BLE 初始化阶段，但供电回路中串联的电流表内阻在 NimBLE 初始化的峰值电流时压降过大，ESP32-S3 内置欠压检测（≈2.43V 阈值）触发，导致约每 3~4 秒**持续循环重启**。

**关键日志序列**（`build-20260413-125005.log`）：

```
12:54:52.158 > INFO  | ??:??:?? 3  [PowerFSM] Init the NimBLE bluetooth module
12:54:55.254 > rst:0xf (BROWNOUT_RST)
12:54:56.086 > INFO  | ??:??:?? 3  [PowerFSM] Init the NimBLE bluetooth module
12:54:59.186 > rst:0xf (BROWNOUT_RST)
...（持续循环）
```

reset reason 从旧日志的 `TWDT` 变为 `BROWNOUT_RST`，触发点固定在 BLE 初始化后约 3 秒。

**修复**：更换电源，移除串联电流表，设备启动正常。

---

## 5. 附录：相关代码位置

| 文件                                             | 行号 | 说明                                                          |
| ------------------------------------------------ | ---- | ------------------------------------------------------------- |
| `variants/esp32s3/eink-ra01s-gp02/lora_sx1262.h` | 12   | `// #define USE_SX1262`（修复点，已注释）                     |
| `variants/esp32s3/eink-ra01s-gp02/variant.h`     | —    | `#define HAS_RADIO 0`                                         |
| `src/mesh/RadioInterface.cpp`                    | 232  | `initLoRa()` 的 `USE_SX1262` 分支入口                         |
| `src/main.cpp`                                   | 1001 | `RECORD_CRITICALERROR(meshtastic_CriticalErrorCode_NO_RADIO)` |
