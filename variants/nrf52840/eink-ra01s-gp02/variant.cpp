/*
  Copyright (c) 2014-2015 Arduino LLC.  All right reserved.
  Copyright (c) 2016 Sandeep Mistry All right reserved.
  Copyright (c) 2018, Adafruit Industries (adafruit.com)

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "variant.h"
#include "SEGGER_RTT.h"
#include "nrf.h"
#include "wiring_constants.h"
#include "wiring_digital.h"

// =============================================================================
// g_ADigitalPinMap
// Maps Arduino logical pin numbers (0..47) to nRF52840 physical GPIO numbers.
// Physical GPIO layout: P0.00–P0.31 → entries 0–31; P1.00–P1.15 → entries 32–47.
//
// Notes:
//   - P0.00 (XL1) and P0.01 (XL2) are the 32MHz HF crystal pins.
//     Mark them 0xff (unusable) to prevent accidental use.
//   - P0.09 (NFC1) and P0.10 (NFC2) are repurposed as GPIO via UICR
//     nfct-pins-as-gpios. The Adafruit bootloader must be compiled with
//     NFCT_PINS_AS_GPIOS=1 (confirmed by the existing NCS UICR config).
//   - P0.18 is the SWD reset pin (UICR gpio-as-nreset). Usable as GPIO
//     in application code (boot behavior only), listed normally.
//   - P0.25, P0.26 are UART TX/RX for Segger RTT/SWD passthrough on some
//     dev kits; on this custom board they are general purpose.
// =============================================================================
const uint32_t g_ADigitalPinMap[] = {
    // P0.00–P0.31
    0xff, // 0  P0.00  XL1  — HF crystal, do not use
    0xff, // 1  P0.01  XL2  — HF crystal, do not use
    2,    // 2  P0.02  PIN_EINK_RES      — E-Ink reset (active-low, idle HIGH)
    3,    // 3  P0.03  PIN_EINK_BUSY     — E-Ink busy output (active-high; nRF pull-down at sleep)
    4,    // 4  P0.04  —
    5,    // 5  P0.05  —
    6,    // 6  P0.06  PIN_BUTTON_ENTER  — button Enter  (ACTIVE_LOW, pull-up)
    7,    // 7  P0.07  —
    8,    // 8  P0.08  PIN_BUTTON_LEFT   — button Left/Back  (ACTIVE_LOW, pull-up)
    9,    // 9  P0.09  PIN_BUTTON_UP     — button Up  (NFC1 as GPIO via UICR NFCPINS; ACTIVE_LOW, pull-up)
    10,   // 10 P0.10  PIN_BUTTON_DOWN   — button Down  (NFC2 as GPIO via UICR NFCPINS; ACTIVE_LOW, pull-up)
    11,   // 11 P0.11  PIN_BUTTON_RIGHT  — button Right  (ACTIVE_LOW, pull-up)
    12,   // 12 P0.12  PIN_GPS_EN        — GP-02 LDO power enable (active HIGH)
    13,   // 13 P0.13  —
    14,   // 14 P0.14  _PIN_LED_GREEN / LED_MSG       — LED Green  (ACTIVE_LOW)
    15,   // 15 P0.15  —
    16,   // 16 P0.16  GP-02 CLKOUT      — GPS clock output (not used by firmware)
    17,   // 17 P0.17  SX126X_BUSY       — LoRa busy output (active-high)
    18,   // 18 P0.18  RESET             — SWD reset (UICR gpio-as-nreset)
    19,   // 19 P0.19  PIN_SPI_SCK       — LoRa SPI0 SCK
    20,   // 20 P0.20  SX126X_DIO1       — LoRa TX/RX done IRQ (active-high)
    21,   // 21 P0.21  —
    22,   // 22 P0.22  PIN_SPI_MOSI      — LoRa SPI0 MOSI
    23,   // 23 P0.23  PIN_SPI_MISO      — LoRa SPI0 MISO
    24,   // 24 P0.24  SX126X_CS         — LoRa chip select (active-low)
    25,   // 25 P0.25  SX126X_RESET      — LoRa reset (active-low, idle HIGH)
    26,   // 26 P0.26  PIN_WIRE_SDA      — I2C SDA (PCF8563 RTC @ 0x51)
    27,   // 27 P0.27  PIN_WIRE_SCL      — I2C SCL (PCF8563 RTC @ 0x51)
    28,   // 28 P0.28  PIN_EINK_DC       — E-Ink data/command (HIGH=data, LOW=cmd)
    29,   // 29 P0.29  PIN_SPI1_MOSI / PIN_EINK_MOSI — E-Ink SPI1 MOSI
    30,   // 30 P0.30  PIN_EINK_CS       — E-Ink chip select (active-low)
    31,   // 31 P0.31  PIN_SPI1_SCK / PIN_EINK_SCLK  — E-Ink SPI1 SCK

    // P1.00–P1.15
    32, // 32 P1.00  —
    33, // 33 P1.01  _PIN_LED_BLUE / LED_PAIRING     — LED Blue  (ACTIVE_LOW)
    34, // 34 P1.02  —
    35, // 35 P1.03  _PIN_LED_RED / LED_EINK_ACTIVITY — LED Red  (ACTIVE_LOW)
    36, // 36 P1.04  GP-02 PPS         — GPS pulse-per-second output (not used by firmware)
    37, // 37 P1.05  PIN_GPS_RESET     — GP-02 hard reset (active HIGH via NPN transistor)
    38, // 38 P1.06  —
    39, // 39 P1.07  —
    40, // 40 P1.08  GPS_TX_PIN / PIN_SERIAL1_TX — UART1 TX → GP-02 RX
    41, // 41 P1.09  GPS_RX_PIN / PIN_SERIAL1_RX — UART1 RX ← GP-02 TX
    42, // 42 P1.10  PIN_BUTTON_FN     — button Fn/Cancel (ACTIVE_LOW, pull-up; hold ≥4s → shutdown)
    43, // 43 P1.11  —
    44, // 44 P1.12  PIN_EINK_EN       — E-Ink panel LDO enable (active HIGH)
    45, // 45 P1.13  —
    46, // 46 P1.14  —
    47, // 47 P1.15  SX126X_POWER_EN   — LoRa module LDO enable (active HIGH)
};

// =============================================================================
// initVariant
// Called by the Arduino framework after SoftDevice init, before setup().
// Only configure hardware that is definitely present in Phase 0.
// DO NOT call sd_power_dcdc_mode_set() — no external LC filter on this board.
// =============================================================================
void initVariant(void)
{
    // --- LEDs ---
    // All three LEDs are GPIO_ACTIVE_LOW: set HIGH (off) before enabling output.
    pinMode(_PIN_LED_RED, OUTPUT);
    ledOff(_PIN_LED_RED);

    pinMode(_PIN_LED_BLUE, OUTPUT);
    ledOff(_PIN_LED_BLUE);

    pinMode(_PIN_LED_GREEN, OUTPUT);
    ledOff(_PIN_LED_GREEN);

    SEGGER_RTT_printf(0, "initVariant: complete\n");
}

// =============================================================================
// variant_shutdown  (weak — optionally called before deep sleep)
// For Phase 0, nothing extra to do. As peripherals are added, add their
// shutdown/tristate here to avoid leakage current (see t-echo variant for
// E-Ink pin tri-stating example).
// =============================================================================
void variant_shutdown(void)
{
    // Turn off all LEDs before deep sleep.
    // PIN_LED1/2/3 are not defined (to avoid framework auto-mapping), so
    // Power.cpp's ledOff(PIN_LEDx) blocks are skipped; we handle it here.
    ledOff(_PIN_LED_RED);
    ledOff(_PIN_LED_BLUE);
    ledOff(_PIN_LED_GREEN);

#if defined(USE_EINK) && defined(PIN_EINK_EN)
    // Anti-leakage sequence before deep sleep.
    //
    // Root cause: GxEPD2 hibernate() ends with a DEEP_SLEEP_MODE command
    // followed by data byte 0x01, leaving DC = OUTPUT HIGH (data mode).
    // The SPI bus retains the last driven state on MOSI as well.
    // When EN cuts the LDO, any MCU pin still driven HIGH backpowers the
    // SSD1680 through its I/O ESD protection diodes (~12 mA per pin).
    //
    // Fix: tristate ALL SSD1680-connected pins BEFORE cutting EN.
    // Order is critical — tristate first, then remove power.
    //
    //   DC   (P0.28) HIGH after hibernate() data phase → primary leakage source
    //   RST  (P0.2)  active-low → idle HIGH           → leakage source
    //   CS   (P0.30) active-low → idle HIGH            → leakage source
    //   MOSI (P0.29) last bit state unknown             → potential leakage
    //   SCK  (P0.31) SPI MODE0 idle = LOW, but tristate for safety
    //   BUSY (P0.3)  input direction, add pull-down so busy_wait() cannot
    //                hang on re-init after wakeup (BUSY floats when unpowered)
    pinMode(PIN_EINK_DC, INPUT);
    pinMode(PIN_EINK_RES, INPUT);
    pinMode(PIN_EINK_CS, INPUT);
    pinMode(PIN_EINK_MOSI, INPUT);
    pinMode(PIN_EINK_SCLK, INPUT);
    pinMode(PIN_EINK_BUSY, INPUT_PULLDOWN);
    // Cut panel LDO last — all driven pins are now high-Z
    digitalWrite(PIN_EINK_EN, LOW);
#endif

#if defined(USE_SX1262) && defined(SX126X_POWER_EN)
    // Anti-leakage sequence before cutting LoRa LDO (deep sleep).
    //
    // Root cause: when VDD collapses, MCU pins still driven HIGH source
    // current through the SX126x I/O ESD protection diodes back into the
    // unpowered rail.
    //
    // Note: cpuDeepSleep() calls SPI.end() BEFORE variant_shutdown(), so
    // SPIM0 pin mux is already released here and pinMode() is effective.
    // We do NOT call SPI.end() again — that is handled by the framework.
    // (This mirrors the E-Ink block above which also skips SPI1.end().)
    //
    // Pins to tristate (all SX1262-connected MCU pins):
    //   RESET (P0.25) active-low → idle HIGH  — primary leakage source
    //   CS    (P0.24) active-low → idle HIGH  — leakage source
    //   MOSI  (P0.22) last SPI bit unknown     — potential leakage
    //   MISO  (P0.23) SX1262 output; released by SPI.end() but tristate explicitly
    //   SCK   (P0.19) SPI MODE0 idle = LOW, but tristate for safety
    //   DIO1  (P0.20) SX1262 IRQ output (active-high); pull-down prevents false IRQ on wakeup
    //   BUSY  (P0.17) module output → INPUT_PULLDOWN prevents busy_wait()
    //                 hang when module is unpowered on next wakeup
    //
    // Order: tristate ALL pins FIRST, cut power LAST.
    pinMode(SX126X_RESET, INPUT);
    pinMode(SX126X_CS, INPUT);
    pinMode(PIN_SPI_MOSI, INPUT);
    pinMode(PIN_SPI_MISO, INPUT);
    pinMode(PIN_SPI_SCK, INPUT);
    pinMode(SX126X_DIO1, INPUT_PULLDOWN);
    pinMode(SX126X_BUSY, INPUT_PULLDOWN);
    digitalWrite(SX126X_POWER_EN, LOW);
#endif
    SEGGER_RTT_printf(0, "variant_shutdown: Entering deep sleep mode\n");
}
