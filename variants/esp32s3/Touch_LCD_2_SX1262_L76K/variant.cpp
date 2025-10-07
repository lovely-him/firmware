// Touch_LCD_2_SX1262_L76K variant.cpp
// 用于硬件初始化

#include "variant.h"
#include <Arduino.h>

#include "esp_log.h"

static const char *TAG = "Touch_LCD_2_SX1262_L76K";

/**
 * @brief 变体初始化函数
 *
 * 此函数在 Arduino 框架启动时自动调用,用于初始化硬件特定的引脚和外设
 * 注意: 此时 Arduino 日志系统可能未初始化,使用 ESP_LOGI
 */
void initVariant()
{
    ESP_LOGI(TAG, "初始化 Touch_LCD_2_SX1262_L76K 变体");

    // 初始化 LoRa 天线控制引脚
    // LORA_CTRL_GPIO (GPIO6) 控制天线使能, 有效电平为低
    pinMode(LORA_CTRL_GPIO, OUTPUT);
    digitalWrite(LORA_CTRL_GPIO, LOW);  // 设置为低电平(有效)

    ESP_LOGI(TAG, "LORA_CTRL_GPIO (GPIO%d) 已初始化为低电平(有效)", LORA_CTRL_GPIO);
}

