// Touch_LCD_2_SX1262_L76K ESP32-S3开发板配置
// 硬件配置:
// - 主控: ESP32-S3
// - 屏幕: 2寸 240x320 ST7789 TFT LCD
// - 触摸: CST816D 电容触控芯片
// - LoRa: SX1262 射频芯片
// - GPS: L76K 模组

#ifndef __TOUCH_LCD_2_SX1262_L76K_VARIANT_H__
#define __TOUCH_LCD_2_SX1262_L76K_VARIANT_H__

// #define LED_PIN 5                        // LED未连接

// I2C配置 (触摸屏)
#define I2C_SDA 48                          // I2C数据线
#define I2C_SCL 47                          // I2C时钟线

// ST7789 TFT LCD配置 (2寸 240x320)
// #define HAS_TFT                          // 用于开启某些功能, 不可以开启,
// #define USE_ST7789                       // 用于开启某些功能, 不可以开启,
// #define VTFT_LEDA (1)                    // 背光控制引脚, 上面宏定义用得到的, 不要开启
// #define VTFT_CTRL (9)                    // TFT电源控制引脚, 上面宏定义用得到的, 不要开启
// #define ST7789_NSS (ST7789_CS)           // 片选引脚, 上面宏定义用得到的, 不要开启
#define ST7789_CS (45)                      // 片选引脚
#define ST7789_RS (42)                      // 数据/命令选择引脚 (DC)
#define ST7789_SDA (38)                     // MOSI数据线
#define ST7789_SCK (39)                     // 时钟线
#define ST7789_MISO (-1)                    // MISO引脚 (允许设置 -1)
#define ST7789_RESET (-1)                   // 复位引脚 (库实际未使用)
#define ST7789_BUSY (-1)                    // BUSY引脚 (库实际未使用)
#define ST7789_BL (1)                       // 背光控制引脚
#define ST7789_SPI_HOST SPI3_HOST           // SPI2 默认被 lora 占用
#define SPI_FREQUENCY 40000000              // 40MHz SPI频率
#define SPI_READ_FREQUENCY 16000000         // 16MHz读取频率

// 屏幕参数
#define TFT_HEIGHT 320                      // TFT屏幕高度像素数，2寸屏幕垂直分辨率
#define TFT_WIDTH 240                       // TFT屏幕宽度像素数，2寸屏幕水平分辨率
#define TFT_OFFSET_X 0                      // 屏幕X轴偏移量，用于校准显示位置
#define TFT_OFFSET_Y 0                      // 屏幕Y轴偏移量，用于校准显示位置
#define TFT_OFFSET_ROTATION 0               // 屏幕旋转偏移角度，0=不旋转
#define SCREEN_ROTATE                       // 启用屏幕旋转功能，支持横竖屏切换
#define SCREEN_TRANSITION_FRAMERATE 5       // 5fps过渡动画
#define BRIGHTNESS_DEFAULT 130              // 默认亮度

// TFT颜色配置 (为了消除警告)
#define TFT_MESH_OVERRIDE COLOR565(0x67, 0xEA, 0x94)  // Meshtastic绿色主题
#define TFT_BLACK COLOR565(0x00, 0x00, 0x00)          // 黑色

// 触摸屏配置 (CST816D)
#define HAS_TOUCHSCREEN 1
#define SCREEN_TOUCH_INT 46                 // 触摸中断引脚
#define TOUCH_I2C_PORT 0                    // I2C端口
#define TOUCH_SLAVE_ADDRESS 0x15            // CST816D I2C地址 (0x15)
// #define WAKE_ON_TOUCH                    // 启用触摸中断唤醒功能

// 按钮配置
#define BUTTON_PIN 0                        // 用户按钮
#define BUTTON_ACTIVE_LOW true              // 按钮低电平有效
#define BUTTON_ACTIVE_PULLUP true           // 启用内部上拉电阻

// 电源管理
// #define VEXT_ENABLE 16                   // 外部电源使能引脚未连接
// #define VEXT_ON_VALUE HIGH               // 外部电源使能引脚有效电平
#define BATTERY_PIN 5                       // 电池电压检测引脚
#define ADC_CHANNEL ADC1_GPIO5_CHANNEL      // 电池电压检测通道
#define ADC_ATTENUATION ADC_ATTEN_DB_12     // 电池电压检测衰减 (12dB, 0-3.3V范围)
#define ADC_MULTIPLIER 3.0                  // 电压分压器比例 (1:3分压)

// SX1262 LoRa模块配置
#define USE_SX1262
#define LORA_DIO0 -1                        // SX1262不使用DIO0
#define LORA_RESET 2                        // 复位引脚
#define LORA_DIO1 17                        // 中断引脚 (GPIO输入)
#define LORA_DIO2 4                         // BUSY引脚
#define LORA_DIO3                           // 未连接

// SPI接口配置 (使用SPI2_HOST)
#define LORA_SCK 8                          // LoRa SPI时钟
#define LORA_MISO 10                        // LoRa MISO引脚
#define LORA_MOSI 7                         // LoRa MOSI引脚
#define LORA_CS 18                          // LoRa片选引脚 (NSS)
// #define HW_SPI1_DEVICE                   // 如果定义了就会使用 SPI1, 默认使用 Arduino 的 SPI, 也就是 SPI2

// 天线控制引脚
#define LORA_CTRL_GPIO 6                    // 天线使能引脚

// SX1262专用配置
#define SX126X_CS LORA_CS                   // SX1262片选引脚
#define SX126X_DIO1 LORA_DIO1               // SX1262中断引脚
#define SX126X_BUSY LORA_DIO2               // SX1262忙状态引脚
#define SX126X_RESET LORA_RESET             // SX1262复位引脚
#define SX126X_DIO2_AS_RF_SWITCH            // 使用DIO2作为RF开关
// #define SX126X_DIO3_TCXO_VOLTAGE 1.8     // TCXO电压, 不使用

// L76K GPS模块配置
#define GPS_L76K
#define GPS_RX_PIN 14                       // GPS接收引脚 (连接到GPS TX)
#define GPS_TX_PIN 9                        // GPS发送引脚 (连接到GPS RX)
#define GPS_BAUDRATE 9600                   // L76K默认波特率
#define GPS_THREAD_INTERVAL 50              // GPS线程间隔
// #define PIN_GPS_PPS -1                   // GPS PPS引脚 (未连接)
// #define PIN_GPS_RESET -1                 // GPS复位引脚 (未连接)
// #define GPS_RESET_MODE LOW               // GPS复位有效电平
// #define PIN_GPS_EN -1                    // GPS使能引脚 (未连接)
// #define GPS_EN_ACTIVE HIGH               // GPS使能有效电平

// 其他配置
// #define HAS_32768HZ 1                    // 32.768kHz晶振
// #define USE_POWERSAVE                    // 启用省电模式
// #define SLEEP_TIME 120                   // 睡眠时间(秒)

// 模块使能
// #define CANNED_MESSAGE_MODULE_ENABLE 1   // 预设消息模块

// 引脚分配已优化，避免冲突
#endif /* __TOUCH_LCD_2_SX1262_L76K_VARIANT_H__ */