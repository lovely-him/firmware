#pragma once
// =============================================================================
// LoRa — Ai-Thinker RA-01S (SX1262/SX1268, sub-GHz)
// =============================================================================
// RA-01S chip note:
//   The physical chip is SX1268 (China-band variant of SX1262).
//   They share the same register map; RadioLib SX1262 driver is fully
//   compatible. USE_SX1262 is correct — do NOT use a hypothetical USE_SX1268.
//
// Oscillator note:
//   RA-01S = crystal oscillator (XO).  DIO3 is NC — do NOT define
//   SX126X_DIO3_TCXO_VOLTAGE; RadioLib will not drive DIO3.
//   RA-01SH = TCXO variant — only uncomment that macro for that variant.
//
// RF switch note:
//   RA-01S uses DIO2 internally as TX/RX antenna switch control.
//   SX126X_DIO2_AS_RF_SWITCH tells RadioLib to set the DIO2 bit in
//   SetDio2AsRfSwitchCtrl; no external MCU pin required.
//
// Frequency / region note:
//   The operating frequency is set at runtime via lora.region in the
//   Meshtastic app (US_915, EU_868, ANZ_433, etc.).  This file does not
//   hard-code a frequency.  Default region at first boot: US_915 (915 MHz).
//   The SX1262/SX1268 chip covers 150–960 MHz.
//   WARNING: the RA-01S PCB antenna is tuned for 868/915 MHz.  Using
//   433 MHz will work electrically but with significant efficiency loss
//   due to antenna impedance mismatch.
//
// Power control:
//   A dedicated LDO (P1.15, active-high) powers the RA-01S module.
//   Meshtastic drives SX126X_POWER_EN HIGH on init and LOW on sleep().
//   Before cutting power we must tristate all MCU-driven SPI/control pins
//   to prevent backpowering through the SX126x I/O ESD protection diodes.
//   SX126X_LORA_PREPOWER_OFF() implements this tristate sequence; it is
//   called by SX126xInterface::sleep() when SX126X_POWER_EN is defined.
//
// SPI bus:
//   Dedicated SPI0 bus (independent from E-Ink SPI1).
//   PIN_SPI_* are defined here (not in peripherals.h) so that all LoRa
//   hardware configuration lives in a single file.
// =============================================================================

#define USE_SX1262

// -----------------------------------------------------------------------------
// SPI0 bus — dedicated to LoRa (write + read; MISO connected)
// Pins: P0.23=MISO  P0.22=MOSI  P0.19=SCK
// Arduino pin number = P0.xx for port 0.
// These macros are read by the Arduino nRF52 core (wiring.cpp / SPI.cpp)
// to initialise the default SPI instance (SPI0 / SPIM0).
// -----------------------------------------------------------------------------
#define PIN_SPI_MISO 23 // P0.23
#define PIN_SPI_MOSI 22 // P0.22
#define PIN_SPI_SCK 19  // P0.19

static const uint8_t SS = 24; // P0.24 — LoRa CS (active-low)
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK = PIN_SPI_SCK;

// -----------------------------------------------------------------------------
// Chip-select + control pins
// -----------------------------------------------------------------------------
#define SX126X_CS 24    // P0.24  NSS/CS   (active-low)
#define SX126X_DIO1 20  // P0.20  DIO1     — TX/RX done IRQ (active-high)
#define SX126X_BUSY 17  // P0.17  BUSY     — module busy output (active-high)
#define SX126X_RESET 25 // P0.25  RESET    — active-low

// -----------------------------------------------------------------------------
// RF switch — DIO2 drives the internal TX/RX switch inside the RA-01S module.
// No external MCU pin needed; RadioLib handles this via SetDio2AsRfSwitchCtrl.
// -----------------------------------------------------------------------------
#define SX126X_DIO2_AS_RF_SWITCH

// -----------------------------------------------------------------------------
// TCXO — NOT present on RA-01S (crystal oscillator variant).
// Do NOT uncomment; RadioLib must not drive DIO3 as TCXO supply.
// (Uncomment only for RA-01SH which has a TCXO.)
// -----------------------------------------------------------------------------
// #define SX126X_DIO3_TCXO_VOLTAGE 1.8

// -----------------------------------------------------------------------------
// Power enable — P1.15 (Arduino pin 47), active-high LDO control.
// SX126xInterface::init()  → HIGH (power on)
// SX126xInterface::sleep() → LOW  (power off, after SX126X_LORA_PREPOWER_OFF)
// -----------------------------------------------------------------------------
#define SX126X_POWER_EN 47 // P1.15

// -----------------------------------------------------------------------------
// Maximum TX power (dBm) — SX1262/SX1268 hardware maximum
// -----------------------------------------------------------------------------
#define SX126X_MAX_POWER 22
