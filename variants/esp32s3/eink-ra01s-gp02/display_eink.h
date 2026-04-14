#pragma once
// =============================================================================
// E-Ink display — not yet connected
// =============================================================================
// Enable checklist:
//   1. Identify the exact GxEPD2 model class for your panel.
//   2. Fill in all pin definitions below.
//   3. Uncomment #define USE_EINK.
//   4. Set HAS_SCREEN = 1 in board_features.h.
//   5. Add GxEPD2 library dependency in platformio.ini (see comment there).
// =============================================================================
//
// If USE_EINK is not defined, configuration.h will:
//   - Set MESHTASTIC_USE_EINK_UI = 0
//   - #undef USE_EINK
// → the entire e-ink code path is excluded from the build automatically.
// =============================================================================

// #define USE_EINK

// -----------------------------------------------------------------------------
// Panel model — pick the matching GxEPD2 class, e.g.:
//   GxEPD2_154_D67   (1.54", 200x200, SSD1681)
//   GxEPD2_213_BN    (2.13", 250x122, SSD1680)
//   GxEPD2_290_T5D   (2.9",  296x128, SSD1680)
// -----------------------------------------------------------------------------
// #define EINK_DISPLAY_MODEL  GxEPD2_xxx
// #define EINK_WIDTH          xxx   // panel pixel width
// #define EINK_HEIGHT         xxx   // panel pixel height

// -----------------------------------------------------------------------------
// SPI and control pins
// -----------------------------------------------------------------------------
// #define PIN_EINK_CS     xx   // SPI chip select
// #define PIN_EINK_DC     xx   // Data / Command
// #define PIN_EINK_RES    xx   // Reset (active LOW)
// #define PIN_EINK_BUSY   xx   // Busy signal (active LOW)
// #define PIN_EINK_SCLK   xx   // SPI clock  (share with LoRa or dedicate)
// #define PIN_EINK_MOSI   xx   // SPI MOSI   (share with LoRa or dedicate)
