---

## `ARDUINO_USB_MODE`

**作用：** 控制 ESP32 Arduino 中 USB 的工作模式。

- 设置为 `1` 时，表示使用**硬件 USB Serial/JTAG（HW CDC）** 模式，即芯片内置的 USB Serial/JTAG 控制器。
- 需要与 `ARDUINO_USB_CDC_ON_BOOT=1` 配合使用，才能将 `Serial` 符号重定向到 HW USB CDC 端口。

```cmake
list(APPEND compile_definitions "ARDUINO_USB_MODE=1")
list(APPEND compile_definitions "ARDUINO_USB_CDC_ON_BOOT=1")
```

**影响：** 设置后，`Serial.begin()` 可以不传波特率参数（因为 USB CDC 不需要波特率），日志输出将通过 USB CDC 而非 UART0。 [[HW CDC 示例](https://github.com/espressif/arduino-esp32/blob/master/idf_component_examples/hw_cdc_hello_world/README.md)]

---

## `ARDUINO_RUNNING_CORE`

**作用：** 指定 **Arduino 主循环（`setup()` 和 `loop()`）运行在哪个 CPU 核心**上。

- 仅在目标 SoC 有 2 个核心时有效（如 ESP32-S3 有双核）。
- 对应 Arduino IDE 中 `Tools -> Arduino Runs On` 选项。

**影响：** 当有重型任务运行时，可以将 Arduino 主循环与其他任务分配到不同核心，避免相互干扰。 [[Arduino Runs On](https://docs.espressif.com/projects/arduino-esp32/en/latest/guides/tools_menu.html#arduino-runs-on)]

---

## `ARDUINO_EVENT_RUNNING_CORE`

**作用：** 指定 **Arduino 事件处理（Events）运行在哪个 CPU 核心**上。

- 同样仅在目标 SoC 有 2 个核心时有效。
- 对应 Arduino IDE 中 `Tools -> Events Run On` 选项。

**影响：** 可以将事件处理与 Arduino 主循环分配到不同核心，提升并发处理能力。 [[Events Run On](https://docs.espressif.com/projects/arduino-esp32/en/latest/guides/tools_menu.html#events-run-on)]

---

## 总结对比

| 宏                           | 控制对象                              | 适用条件                                     |
| ---------------------------- | ------------------------------------- | -------------------------------------------- |
| `ARDUINO_USB_MODE`           | USB 工作模式（HW CDC vs USB OTG）     | 支持 USB 外设的芯片（如 ESP32-S3、ESP32-C3） |
| `ARDUINO_RUNNING_CORE`       | Arduino `setup()`/`loop()` 运行的核心 | 双核 SoC（如 ESP32-S3）                      |
| `ARDUINO_EVENT_RUNNING_CORE` | Arduino 事件处理运行的核心            | 双核 SoC（如 ESP32-S3）                      |

> ⚠️ 知识来源中对 `ARDUINO_RUNNING_CORE` 和 `ARDUINO_EVENT_RUNNING_CORE` 的具体取值（如 `0` 或 `1` 对应哪个核心）没有详细说明，建议参考 [arduino-esp32 官方文档](https://docs.espressif.com/projects/arduino-esp32/en/latest/guides/tools_menu.html) 获取更多细节。

---
