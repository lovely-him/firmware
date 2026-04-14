#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// =============================================================================
// eink-ra01s-gp02  ESP32-S3 Arduino Standard Pin Mapping
// =============================================================================
// This file defines the standard pin names used by the Arduino framework and
// any library that calls SPI.begin() / Wire.begin() / Serial.begin() without
// explicit pin arguments.
//
// Fill in the TODO values to match your actual PCB wiring.
// These constants must be consistent with the defines in lora_sx1262.h,
// gps_gp02.h, and peripherals.h.
// =============================================================================

// -----------------------------------------------------------------------------
// USB
// N16R8 uses the ESP32-S3 native USB OTG peripheral (TinyUSB CDC).
// VID/PID match the default Espressif TinyUSB CDC device.
// -----------------------------------------------------------------------------
#define USB_VID 0x303a
#define USB_PID 0x1001

// -----------------------------------------------------------------------------
// UART  — default Serial1 / HardwareSerial mapping
// Point these at the GP-02 UART when GPS is wired up.
// -----------------------------------------------------------------------------
static const uint8_t TX = 17; // TODO: ESP32-S3 UART TX → GP-02 RX
static const uint8_t RX = 18; // TODO: ESP32-S3 UART RX ← GP-02 TX

// -----------------------------------------------------------------------------
// SPI  — default SPI bus (shared with LoRa)
// Must match LORA_SCK / LORA_MOSI / LORA_MISO / LORA_CS in lora_sx1262.h.
// -----------------------------------------------------------------------------
static const uint8_t SS = 10;   // TODO: → RA-01S NSS  (= LORA_CS)
static const uint8_t MOSI = 11; // TODO: → RA-01S MOSI (= LORA_MOSI)
static const uint8_t MISO = 13; // TODO: → RA-01S MISO (= LORA_MISO)
static const uint8_t SCK = 12;  // TODO: → RA-01S SCK  (= LORA_SCK)

// -----------------------------------------------------------------------------
// I2C  — default Wire bus
// Must match I2C_SDA / I2C_SCL in peripherals.h when defined.
// -----------------------------------------------------------------------------
static const uint8_t SDA = 8; // TODO
static const uint8_t SCL = 9; // TODO

// -----------------------------------------------------------------------------
// Analog input  — ESP32-S3 ADC-capable GPIOs (GPIO 1–20)
// A0–A19 map 1:1 to GPIO 1–20.
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Capacitive touch  — ESP32-S3 touch-capable GPIOs (GPIO 1–14)
// -----------------------------------------------------------------------------
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
