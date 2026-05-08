#pragma once
// =============================================================================
// GPS — Ai-Thinker GP-02 (L76K compatible, standard NMEA 9600 bps)
// =============================================================================
// Enable checklist (when HAS_GPS is set to 1 in variant.h):
//   1. Fill in GPS_RX_PIN and GPS_TX_PIN below.
//   2. Set HAS_GPS = 1 in variant.h.
//   3. Uncomment optional control pins as needed.
// =============================================================================
//
// Safety note on optional control pins:
//   Do NOT define control pins as -1. GPS.cpp has bare #ifdef guards that
//   call pinMode(PIN, ...) even when the value is -1, which corrupts GPIO
//   state. Leave optional pins commented until a real GPIO is assigned.
// =============================================================================

// -----------------------------------------------------------------------------
// UART data pins  (required when HAS_GPS = 1)
// GP-02 TX → P1.09 (nRF52840 UART RX)   Arduino pin = 32 + 9 = 41
// GP-02 RX ← P1.08 (nRF52840 UART TX)   Arduino pin = 32 + 8 = 40
// Confirmed from gnss_gp02.overlay (uart1: TX=P1.08, RX=P1.09)
// -----------------------------------------------------------------------------
#define GPS_RX_PIN (32 + 9) // P1.09 — nRF52840 UART RX ← GP-02 TX
#define GPS_TX_PIN (32 + 8) // P1.08 — nRF52840 UART TX → GP-02 RX

// When defined, map to Serial1:
#define PIN_SERIAL1_RX GPS_RX_PIN
#define PIN_SERIAL1_TX GPS_TX_PIN

// -----------------------------------------------------------------------------
// Optional control pins
// -----------------------------------------------------------------------------

// LDO power enable — P0.12, active HIGH (confirmed from gnss_gp02.overlay)
#define PIN_GPS_EN (12) // P0.12
#define GPS_EN_ACTIVE HIGH

// Hard reset — P1.05, active HIGH via NPN transistor (high=reset, low=idle)
// GPS_RESET_MODE defaults to HIGH in GPS.cpp — matches this circuit.
#define PIN_GPS_RESET (32 + 5) // P1.05

// Standby / force-on — active LOW by default (GPS_STANDBY_ACTIVE in GPS.cpp)
// #define PIN_GPS_STANDBY xx

// Pulse-per-second output from GP-02 (not in overlay, confirmed by user)
// Meshtastic only calls pinMode(PIN_GPS_PPS, INPUT); no IRQ/RTC use currently.
// #define PIN_GPS_PPS     xx

// -----------------------------------------------------------------------------
// Board-specific GP-02 signals
// No Meshtastic framework support; implement in variant.cpp if needed.
// -----------------------------------------------------------------------------
// GP-02 VRTC    : external 3.3 V backup power enable (RTC memory retention)
// GP-02 FORCE_ON: wake from backup mode
// GP-02 GNSS_SEL: satellite constellation selection (GPS / GLONASS / BDS …)

// -----------------------------------------------------------------------------
// Baud rate
// GP-02 default is 9600 bps, which matches the Meshtastic framework default.
// Only define GPS_BAUDRATE if a non-default rate is needed.
// -----------------------------------------------------------------------------
// #define GPS_BAUDRATE  9600
