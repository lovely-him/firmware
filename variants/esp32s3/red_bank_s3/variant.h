
#pragma once

// 模块构建开关

// 模块功能开关
#undef  HAS_WIRE
#define HAS_WIRE 0              // 没有iic设备

// 模块类型选择
#define USE_SX1262              // lora 设备型号选择

// 按键引脚接口
#define BUTTON_PIN      (1)     // 按键引脚

// GNSS 引脚定义
#define GPS_RX_PIN      (18)    // 串口
#define GPS_TX_PIN      (17)    // 串口

// #define GPS_WAKEUP_PIN  (39) // 多余输出，GNSS模块唤醒主机,高电平有效
// #define PIN_GPS_PPS     (-1) // 多余输出，定位成功后默认输出1hz脉冲，没接不要定义
// #define GPS_BOOT_MODE   (40) // 多余输入，Boot模式控制引脚，低电平进入Boot模式

#define PIN_GPS_EN      (21)    // GNSS模块电源控制引脚
#define GPS_EN_ACTIVE   (HIGH)  // 输出使能为高电平
#define PIN_GPS_RESET   (41)    // GNSS模块复位引脚
#define GPS_RESET_MODE  (HIGH)  // 复位为低电平，但是这个引脚接了一个NPN，高电平导通，导通就是低电平复位，所以是高电平复位

// 开源库中还有一些其他 GPS 引脚定义，未列举

// E-Ink 墨水屏引脚定义
#define PIN_EINK_BS1    (4)     // 4/3线SPI选择引脚，低电平四线，高电平三线
#define PIN_EINK_CS     (15)    // SPI片选
#define PIN_EINK_BUSY   (48)    // 忙状态输出引脚。当 BUSY 为高电平时，芯片的操作不应中断，且不应向模块发出任何命令
#define PIN_EINK_DC     (16)    // 高电平数据/低电平命令选择
#define PIN_EINK_RES    (47)    // 复位引脚
#define PIN_EINK_SCLK   (14)    // 时钟线
#define PIN_EINK_MOSI   (11)    // 输入输出数据线

// #define PIN_EINK_EN  (xx)    // 背光控制引脚
// #define RESET_OLED   (PIN_EINK_RES) // OLED 显示器的复位引脚，低电平激活
#define VEXT_ENABLE     (38)    // OLED 显示器的电源使能引脚
#define VEXT_ON_VALUE   (HIGH)  // 使能为高电平

// Button 按钮引脚定义
#define BUTTON_KEY1_PIN (1)     // ADC 按键1
#define BUTTON_KEY2_PIN (2)     // ADC 按键2

// Lora SPI通信引脚定义
#define LORA_EN         (42)    // Lora 模块电源控制引脚
#define LORA_SCK        (5)     // SPI时钟
#define LORA_MISO       (3)     // SPI数据输入
#define LORA_MOSI       (6)     // SPI数据输出
#define LORA_CS         (7)     // SPI片选

// Lora 状态中断引脚定义
#define LORA_DIO0       (-1)    // a No connect on the SX1262 module
#define LORA_RESET      (8)
#define LORA_DIO1       (10)    // SX1262 IRQ
#define LORA_DIO2       (9)     // SX1262 BUSY
#define LORA_DIO3               // Not connected on PCB, but internally on the TTGO SX1262, if DIO3 is high the TXCO is enabled

#define LORA_ANT_900    (12)    // 天线选择引脚 US United States 902.0 - 928.0
#define LORA_ANT_413    (13)    // 天线选择引脚 CN China 470.0 - 510.0

// Lora 模块类型定义
#ifdef USE_SX1262
#define SX126X_POWER_EN (LORA_EN)       // SX1262 电源控制引脚
#define SX126X_CS       LORA_CS         // FIXME - we really should define LORA_CS instead
#define SX126X_DIO1     LORA_DIO1       // LORA_DIO1
#define SX126X_BUSY     LORA_DIO2       // LORA_DIO2
#define SX126X_RESET    LORA_RESET      // LORA_RESET
#define SX126X_DIO2_AS_RF_SWITCH
// #define SX126X_DIO3_TCXO_VOLTAGE 1.8
#endif


