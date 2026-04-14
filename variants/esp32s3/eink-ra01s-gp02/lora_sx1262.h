#pragma once
// =============================================================================
// LoRa — Ai-Thinker RA-01S (SX1262, no TCXO, external crystal)
// =============================================================================
// Key decisions:
//   - No TCXO        → do NOT define SX126X_DIO3_TCXO_VOLTAGE
//   - No external PA → do NOT define SX126X_TXEN / SX126X_RXEN
//                      SX126xInterface.cpp defaults both to RADIOLIB_NC
//   - DIO2_AS_RF_SWITCH → RA-01S routes DIO2 to the built-in RF switch
// =============================================================================

// #define USE_SX1262  // Commented out: HAS_RADIO=0, pins not yet wired.
//                   // This file is included only to set all LORA_* pins = -1,
//                   // preventing architecture.h from substituting ESP32-classic
//                   // defaults (GPIO 5/18/19/27) that trample OPI PSRAM → TG1WDT.

// -----------------------------------------------------------------------------
// SPI bus pins  (fill in actual GPIO numbers)
// -----------------------------------------------------------------------------
#define LORA_SCK (-1)  // TODO: → RA-01S SCK
#define LORA_MISO (-1) // TODO: → RA-01S MISO
#define LORA_MOSI (-1) // TODO: → RA-01S MOSI
#define LORA_CS (-1)   // TODO: → RA-01S NSS

// SX126x driver aliases
#define SX126X_CS LORA_CS
#define SX126X_SCK LORA_SCK
#define SX126X_MOSI LORA_MOSI
#define SX126X_MISO LORA_MISO

// -----------------------------------------------------------------------------
// Control pins  (fill in actual GPIO numbers)
// -----------------------------------------------------------------------------
#define SX126X_RESET (-1) // TODO: → RA-01S NRST
#define SX126X_BUSY (-1)  // TODO: → RA-01S BUSY
#define SX126X_DIO1 (-1)  // TODO: → RA-01S DIO1

// RA-01S uses DIO2 as the internal RF switch control line.
// No external TXEN/RXEN wiring is needed.
#define SX126X_DIO2_AS_RF_SWITCH

// -----------------------------------------------------------------------------
// Power / gain
// -----------------------------------------------------------------------------
// 22 dBm is the SX1262 hardware ceiling; RA-01S does not add external gain.
#define SX126X_MAX_POWER 22

// TX_GAIN_LORA: dBm subtracted for PA/cable losses.
// Leave undefined here (configuration.h defaults it to 0) unless losses exist.
