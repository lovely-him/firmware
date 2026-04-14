#pragma once
// =============================================================================
// Peripheral pins — I2C, LED, NeoPixel, Battery ADC
// =============================================================================

// -----------------------------------------------------------------------------
// I2C bus  — reserved for future sensors / RTC / OLED
// -----------------------------------------------------------------------------
// #define I2C_SDA  xx   // TODO
// #define I2C_SCL  xx   // TODO

// -----------------------------------------------------------------------------
// Status LED
// -----------------------------------------------------------------------------
// #define LED_PIN  xx   // TODO: onboard LED GPIO

// -----------------------------------------------------------------------------
// NeoPixel / WS2812
// -----------------------------------------------------------------------------
// #define HAS_NEOPIXEL
// #define NEOPIXEL_COUNT  1
// #define NEOPIXEL_DATA   xx   // TODO: data GPIO

// -----------------------------------------------------------------------------
// Battery voltage ADC
// Requires a resistor divider feeding an ADC-capable GPIO.
// ADC_MULTIPLIER accounts for the divider ratio so the firmware computes
// the true battery voltage: V_bat = ADC_reading * ADC_MULTIPLIER.
// -----------------------------------------------------------------------------
// #define BATTERY_PIN      xx    // TODO: ADC GPIO
// #define ADC_MULTIPLIER   2.0   // adjust to match actual divider
