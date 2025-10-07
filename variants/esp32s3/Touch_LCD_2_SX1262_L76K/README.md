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
- MISO: GPIO40
- RST (复位): 未使用
- BL (背光): GPIO1

### CST816D 触摸屏 (I2C接口)
- SDA: GPIO48
- SCL: GPIO47
- INT (中断): GPIO46
- I2C地址: 0x15

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

1. **SPI分离**: ST7789使用SPI2_HOST，SX1262使用独立的SPI引脚，避免冲突
2. **I2C地址**: CST816D触摸芯片的I2C地址为0x15
3. **GPS波特率**: L76K默认使用9600波特率
4. **电源管理**: 使用VEXT_ENABLE引脚控制外部模块电源
5. **触摸校准**: 首次使用时可能需要校准触摸屏

## 功能特性

- ✅ 2寸彩色TFT显示
- ✅ 电容触摸操作
- ✅ LoRa网状网络通信
- ✅ GPS/北斗定位
- ✅ 电池电压监测
- ✅ 省电模式
- ✅ 用户按钮交互
