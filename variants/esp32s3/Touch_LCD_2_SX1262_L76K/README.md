# Touch_LCD_2_SX1262_L76K ESP32-S3开发板配置

## 硬件规格

- **主控芯片**: ESP32-S3
- **屏幕**: 2寸 240×320 分辨率 ST7789 驱动芯片的 LCD 彩屏
- **触摸**: CST816D 电容触控芯片
- **LoRa**: SX1262 射频芯片的 LoRa 模组
- **GPS**: L76K 模组，支持 GPS 和 BD 等定位功能

## 引脚分配

### ST7789 TFT LCD (SPI接口)
- CS (片选): GPIO45
- DC (数据/命令): GPIO42
- MOSI: GPIO38
- SCK: GPIO39
- MISO: 未使用
- RST (复位): 未使用
- BL (背光): GPIO1

### CST816D 触摸屏 (I2C接口)
- SDA: GPIO48
- SCL: GPIO47
- INT (中断): GPIO46
- I2C地址: 0x15
- **驱动方式**: 使用LovyanGFX的Touch_CST816S驱动 (CST816D与CST816S完全兼容)
- **注意**: 已在TFTDisplay.cpp中配置使用lgfx::Touch_CST816S类

### SX1262 LoRa模块 (SPI接口)
- CS (NSS): GPIO18
- MOSI: GPIO7
- SCK: GPIO8
- MISO: GPIO10
- RST: GPIO2
- DIO1 (中断): GPIO17
- BUSY: GPIO4
- CTRL (天线使能): GPIO6

### L76K GPS模块 (UART接口)
- TX: GPIO9 (连接到GPS RX)
- RX: GPIO14 (连接到GPS TX)
- RST: GPIO5
- PPS: 未连接
- EN: 未连接

### 其他引脚
- 用户按钮: GPIO0
- LED指示: 未连接
- 电池检测: GPIO5
- 外部电源使能: 未连接

## 构建配置

使用以下命令构建固件:

```bash
pio run -e touch_lcd_2_sx1262_l76k
```

## 参考变体

本配置参考了以下现有变体:
- `variants/esp32s3/t-deck/` - ST7789 TFT配置 (编译失败)
- `variants/esp32s3/heltec_vision_master_t190/` - ST7789 TFT配置 (编译成功)
- `variants/esp32s3/t-watch-s3/` - ST7789 TFT配置 (运行成功)
- `variants/esp32s3/heltec_v3/` - SX1262 LoRa配置
- `variants/esp32s3/heltec_v4/` - L76K GPS配置
- `variants/esp32s3/rak_wismesh_tap_v2/` - 触摸屏配置

## 注意事项

1. `arduino` 的 默认 `spi(单元2)` 接到 `lora`, 默认 `i2c(单元?)` 共用, 默认 `uart(单元0)` 接到 `gps`.
2. `gps` 的 `rx/tx` 引脚不能接上拉电阻, 注意引脚的选取. 其他引脚也是同理. 不要连接其他东西避免干扰.
3. `gps` 的供电不能太低, 无法正常工作. 注意供电.
4. 目前测试 `lovyan03/LovyanGFX@1.2.0` 不能用新版, 固定版本才能编译.
5. 目前测试不能添加调试等功能, 否则编译失败. 连 `CONFIG_ARDUHAL_LOG_COLORS` 都不可以? 所以其他多余的配置不要添加.
6. 开源工程相对混乱, 各种命名意思相同的宏定义乱飞, 比如不能定义 `USE_ST7789` 和 `HAS_TFT`, 开启屏幕的宏定义其实是 `ST7789_CS`. 多余的宏定义不要添加.
7. 目前发现 `seeed-sensecap-indicator` 和 `t-deck` 的屏幕和触摸配置都是自定义的 `bb_captouch` 库, 在库中加了自己专门的配置, 所以不能直接用.
8. 电池电压检测注意 `ADC_MULTIPLIER` 和 `ADC_ATTENUATION` 的配置, 否则测量不对, 还有引脚 `BATTERY_PIN` 和通道 `ADC_CHANNEL` 的设置.
9. 发现有可重载的初始化函数 `lateInitVariant()`, 目前只有三个配置用到, 类似 `variant.cpp` 中的 `initVariant()`, 不一样的是前者是硬件初始化到一定阶段后执行. `t_deck_pro` 配置的触摸功能就重载了.
10. 一步步查看触摸驱动和屏幕的驱动后, 大致了解了配置, 只需要在 `src\graphics\TFTDisplay.cpp` 中配置即可. 库中本来就有这个触摸芯片系列 `工程目录\.pio\libdeps\touch_lcd_2_sx1262_l76k\LovyanGFX\src\lgfx\v1\touch\Touch_CST816S.cpp` .

```cpp
class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;
#if HAS_TOUCHSCREEN
#if defined(T_WATCH_S3) || defined(ELECROW)
    lgfx::Touch_FT5x06 _touch_instance;
#elif defined(TOUCH_LCD_2_SX1262_L76K)
    lgfx::Touch_CST816S _touch_instance;  // CST816D兼容CST816S驱动
#else
    lgfx::Touch_GT911 _touch_instance;
#endif
#endif
}
// ...
```



## 功能特性

- ✅ 2寸彩色TFT显示
- ✅ 电容触摸操作
- ✅ LoRa网状网络通信
- ✅ GPS/北斗定位
- ✅ 电池电压监测
- ✅ 省电模式
- ✅ 用户按钮交互
