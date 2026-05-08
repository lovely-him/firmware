#pragma once
// =============================================================================
// E-Ink display — GDEM0213B74 (SSD1680, 2.13", 250×128 physical / 250×122 visible)
// =============================================================================
// Dedicated SPI1 bus, independent from LoRa SPI0:
//   SCLK  → P0.31  (PIN_SPI1_SCK)
//   MOSI  → P0.29  (PIN_SPI1_MOSI)
//   CS    → P0.30  (PIN_EINK_CS,  active-low)
//   DC    → P0.28  (PIN_EINK_DC,  high=data / low=command)
//   RES   → P0.2   (PIN_EINK_RES, active-low)
//   BUSY  → P0.3   (PIN_EINK_BUSY, active-high; no external pull-down;
//                   nRF52 internal pull-down applied at boot and deep sleep)
//   PWR   → P1.12  (PIN_EINK_EN,  active-high; controls panel power rail)
// =============================================================================

// USE_EINK is NOT defined here. It is added via -D USE_EINK in the Screen
// variant's build_flags (platformio.ini [env:eink-ra01s-gp02]), so the InkHUD
// variant (which shares this header via variant.h) does not inherit it.

// -----------------------------------------------------------------------------
// Panel model
// GxEPD2_213_B74: driver specifically for GDEM0213B74 (SSD1680).
// Uses the OTP built-in LUT for partial updates — correct for this panel.
// GxEPD2_213_BN targets DEPG0213BN (DKE) and injects a foreign LUT that
// causes partial-update ghosting / content overlap on GDEM0213B74.
// Physical: WIDTH=128 gate lines, HEIGHT=250 source lines.
// After setRotation(3): display coordinate width=250, height=128.
// -----------------------------------------------------------------------------
#define EINK_DISPLAY_MODEL GxEPD2_213_B74
#define EINK_WIDTH 250 // display width  after rotation-3  (source lines)
#define EINK_HEIGHT                                                                                                              \
    122 // display height after rotation-3  (visible gate lines; physical=128 for 8-bit alignment but 6 are not connected to
        // display material)

// -----------------------------------------------------------------------------
// Screen rotation — adjust to match physical installation orientation:
//   0 → 250w × 122h  landscape, FPC at bottom
//   1 → 122w × 250h  portrait
//   2 → 250w × 122h  landscape, FPC at top (180°)
//   3 → 122w × 250h  portrait, rotated 180°
// -----------------------------------------------------------------------------
#define EINK_ROTATION 3

// -----------------------------------------------------------------------------
// UI tuning
// EINK_NAVBAR_DURATION_MS: how long the navigation bar stays visible after a
// page switch before auto-hiding. UIRenderer.cpp checks;
// Both macros are defined in this file, so the custom duration is always active.
// -----------------------------------------------------------------------------
#define EINK_NAVBAR_DURATION_MS 5000

// -----------------------------------------------------------------------------
// Dynamic refresh control
// USE_EINK_DYNAMICDISPLAY is defined in platformio.ini (must be a global -D flag
// so the external GxEPD2 library can see it — it does not include configuration.h).
// The remaining EINK_* tuning macros are safe here; all consumers include
// configuration.h → variant.h → display_eink.h.
// -----------------------------------------------------------------------------
#define EINK_LIMIT_FASTREFRESH (20)                           // Force full refresh after N consecutive fast-refreshes (ps:5)
#define EINK_LIMIT_GHOSTING_PX (EINK_WIDTH * EINK_HEIGHT / 2) // Trigger full refresh when ghost pixels exceed N (ps:2000)
#define EINK_BACKGROUND_USES_FAST                             // Use FAST refresh for BACKGROUND frames too (until limit)
#define EINK_HASQUIRK_GHOSTING                                // Force full refresh on wake to clear "Screen Paused" ghost

// -----------------------------------------------------------------------------
// Panel power enable
// P1.12 → Arduino pin 44;  active-high, controls display module power rail.
// Must be HIGH before SPI1.begin() / GxEPD2 init.
// Must go LOW after RST and CS are set to INPUT (anti-leakage).
// -----------------------------------------------------------------------------
#define PIN_EINK_EN 44 // P1.12

// -----------------------------------------------------------------------------
// SPI1 bus — dedicated to e-ink (write-only; MISO not connected)
// The Arduino framework constructs SPI1 at startup using PIN_SPI1_* macros;
// SPI1.begin() has no pin-override overload, so these macros are mandatory.
//   P0.31 → Arduino 31,  P0.29 → Arduino 29
// -----------------------------------------------------------------------------
#define PIN_SPI1_MISO (-1) // write-only; maps to NRFX_SPIM_PIN_NOT_USED
#define PIN_SPI1_SCK 31    // P0.31
#define PIN_SPI1_MOSI 29   // P0.29

// -----------------------------------------------------------------------------
// SPI control and signal pins
//   P0.30 → Arduino 30,  P0.28 → Arduino 28
//   P0.2  → Arduino 2,   P0.3  → Arduino 3
// -----------------------------------------------------------------------------
#define PIN_EINK_CS 30  // P0.30  chip select (active-low)
#define PIN_EINK_DC 28  // P0.28  data/command
#define PIN_EINK_RES 2  // P0.2   reset (active-low; idle HIGH → anti-leakage target)
#define PIN_EINK_BUSY 3 // P0.3   busy (active-high; no external pull-down)
#define PIN_EINK_SCLK PIN_SPI1_SCK
#define PIN_EINK_MOSI PIN_SPI1_MOSI
