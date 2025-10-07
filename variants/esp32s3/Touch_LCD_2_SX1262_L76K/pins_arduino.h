#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>
#include "soc/soc_caps.h"

// =============================================================================
// Touch_LCD_2_SX1262_L76K ESP32-S3 开发板 Arduino 标准引脚映射
// =============================================================================
// 此文件定义 Arduino 框架需要的标准引脚名称
// 硬件特定配置请参考 variant.h 文件
// =============================================================================

// ==== USB 配置 ====
#define USB_VID 0x303a
#define USB_PID 0x1001

// ==== LED 配置 ====
// 注意: 本开发板没有连接 LED
// #define LED_BUILTIN -1

// ==== 串口配置 ====
static const uint8_t TX = 21;                         // 默认串口 TX 引脚
static const uint8_t RX = 16;                         // 默认串口 RX 引脚

// ==== I2C 配置 ====
static const uint8_t SDA = 48;                        // I2C 数据线
static const uint8_t SCL = 47;                        // I2C 时钟线

// ==== SPI 配置 ====
static const uint8_t SS = 18;                         // SPI 片选引脚
static const uint8_t MOSI = 7;                        // SPI MOSI 数据引脚
static const uint8_t MISO = 10;                       // SPI MISO 数据引脚
static const uint8_t SCK = 8;                         // SPI 时钟引脚

// ==== 模拟输入引脚映射 ====
static const uint8_t A0 = 1;
static const uint8_t A1 = 2;
static const uint8_t A2 = 3;
static const uint8_t A3 = 4;
static const uint8_t A4 = 5;
static const uint8_t A5 = 6;
static const uint8_t A6 = 7;
static const uint8_t A7 = 8;
static const uint8_t A8 = 9;
static const uint8_t A9 = 10;
static const uint8_t A10 = 11;
static const uint8_t A11 = 12;
static const uint8_t A12 = 13;
static const uint8_t A13 = 14;
static const uint8_t A14 = 15;
static const uint8_t A15 = 16;
static const uint8_t A16 = 17;
static const uint8_t A17 = 18;
static const uint8_t A18 = 19;
static const uint8_t A19 = 20;

// ==== 触摸输入引脚映射 ====
static const uint8_t T1 = 1;
static const uint8_t T2 = 2;
static const uint8_t T3 = 3;
static const uint8_t T4 = 4;
static const uint8_t T5 = 5;
static const uint8_t T6 = 6;
static const uint8_t T7 = 7;
static const uint8_t T8 = 8;
static const uint8_t T9 = 9;
static const uint8_t T10 = 10;
static const uint8_t T11 = 11;
static const uint8_t T12 = 12;
static const uint8_t T13 = 13;
static const uint8_t T14 = 14;

#endif /* Pins_Arduino_h */
