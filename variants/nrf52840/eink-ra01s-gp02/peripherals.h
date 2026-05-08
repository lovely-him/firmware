#pragma once
#include <stdint.h> // uint8_t — satisfies IntelliSense; already provided by Arduino at compile time
// =============================================================================
// Peripheral pins — LEDs, buttons, SPI, I2C, battery ADC
// =============================================================================

// -----------------------------------------------------------------------------
// LEDs
// From DTS (GPIO_ACTIVE_LOW → LED_STATE_ON = 0):
//   led0: gpio1 3  → P1.03 → Arduino pin 32+3 = 35  (Red)
//   led1: gpio1 1  → P1.01 → Arduino pin 32+1 = 33  (Blue)
//   led2: gpio0 14 → P0.14 → Arduino pin       14   (Green)
//
// IMPORTANT: PIN_LED1/PIN_LED2/PIN_LED3 are intentionally NOT defined here.
// nrf52/architecture.h auto-maps PIN_LED1 → LED_POWER when LED_POWER is absent,
// which would let StatusLEDModule reclaim the red LED and conflict with
// LED_EINK_ACTIVITY.  We use private names instead and define LED_POWER as
// a sentinel to block the auto-mapping.
// -----------------------------------------------------------------------------
#define _PIN_LED_RED (32 + 3)   // P1.03  Red   LED
#define _PIN_LED_BLUE (32 + 1)  // P1.01  Blue  LED
#define _PIN_LED_GREEN (0 + 14) // P0.14  Green LED

#define LED_STATE_ON 0 // All LEDs are GPIO_ACTIVE_LOW

#define LED_PAIRING _PIN_LED_BLUE
#define LED_MSG _PIN_LED_GREEN
#define LED_EINK_ACTIVITY _PIN_LED_RED

// -----------------------------------------------------------------------------
// Buttons
// From DTS (GPIO_PULL_UP GPIO_ACTIVE_LOW), Arduino pin numbering:
//   Enter  : gpio0  6  → P0.06 → pin  6  (was reserved for UART TX in NCS, free here)
//   Left   : gpio0  8  → P0.08 → pin  8  (was reserved for UART RX in NCS, free here)
//   Up     : gpio0  9  → P0.09 → pin  9  (NFC1 pin, repurposed via UICR NFCPINS)
//   Down   : gpio0 10  → P0.10 → pin 10  (NFC2 pin, repurposed via UICR NFCPINS)
//   Right  : gpio0 11  → P0.11 → pin 11
//   Cancel : gpio1 10  → P1.10 → pin 42
//
// All buttons are ACTIVE_LOW with hardware pull-up (INPUT_PULLUP in software).
// The NFC pins P0.09/P0.10 are reconfigured as GPIO by the Adafruit bootloader
// via UICR CONFIG0.NFCPINS = 1; no user action required.
//
// NOTE: BUTTON_PIN / ALT_BUTTON_PIN / CANCEL_BUTTON_PIN are intentionally NOT
// defined here.  SixKeyNavInput handles all 6 buttons directly (including Cancel
// modifier and long-press shutdown), bypassing the ButtonThread singletons.
// -----------------------------------------------------------------------------
#define HAS_BUTTON 1

#define PIN_BUTTON_ENTER (0 + 6)  // P0.06  — Confirm / Select
#define PIN_BUTTON_LEFT (0 + 8)   // P0.08  — Navigate Left / Back
#define PIN_BUTTON_UP (0 + 9)     // P0.09  — Navigate Up
#define PIN_BUTTON_DOWN (0 + 10)  // P0.10  — Navigate Down
#define PIN_BUTTON_RIGHT (0 + 11) // P0.11  — Navigate Right
#define PIN_BUTTON_FN (32 + 10)   // P1.10  — Fn: Combo modifier (hold) / Shutdown (hold ≥4s)

// -----------------------------------------------------------------------------
// SPI bus
// SPI0 (default SPI): LoRa (SX1262) — pins and Arduino aliases defined in lora_sx1262.h
// SPI1: E-Ink (SSD1680) — pins defined in display_eink.h (PIN_SPI1_*)
// -----------------------------------------------------------------------------
#define SPI_INTERFACES_COUNT 2 // SPI0: LoRa (P0.19/22/23); SPI1: E-Ink (P0.31/P0.29)

// PIN_SPI_MISO/MOSI/SCK and SS/MOSI/MISO/SCK aliases are defined in lora_sx1262.h.

// -----------------------------------------------------------------------------
// I2C bus — PCF8563 RTC (and future sensors)
// P0.26 = SDA, P0.27 = SCL. External pull-ups present on PCB.
// Arduino pin number = P0.xx directly for port 0.
// -----------------------------------------------------------------------------
#define WIRE_INTERFACES_COUNT 1

#define PIN_WIRE_SDA 26 // P0.26 — I2C SDA (PCF8563)
#define PIN_WIRE_SCL 27 // P0.27 — I2C SCL (PCF8563)

// PCF8563 RTC at I2C address 0x51.
// Enables the PCF8563_RTC branch in ScanI2CTwoWire.cpp + RTC.cpp:
//   - readFromRTC(): reads datetime → RTCQualityDevice on boot
//   - perhapsSetRTC(): writes datetime back when GPS/NTP improves quality
// No INT pin connected; no alarm functionality used.
// CLKOUT (P0.16) is not used by Meshtastic — hardware pin is safe to ignore.
#define PCF8563_RTC 0x51
