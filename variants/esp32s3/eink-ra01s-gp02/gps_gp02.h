#pragma once
// =============================================================================
// GPS — Ai-Thinker GP-02 (L76K compatible, standard NMEA 9600 bps)
// =============================================================================
// Enable checklist (when HAS_GPS is set back to 1 in board_features.h):
//   1. Fill in GPS_RX_PIN and GPS_TX_PIN below.
//   2. Set HAS_GPS = 1 in board_features.h.
//   3. Uncomment optional control pins as needed.
// =============================================================================
//
// Safety note on control pins:
//   Do NOT define control pins as -1. GPS.cpp has a bare #ifdef guard that
//   calls pinMode(PIN, ...) even if the value is -1. Leave them commented
//   until an actual GPIO is assigned.
// =============================================================================

// -----------------------------------------------------------------------------
// UART data pins  (required when HAS_GPS = 1; comment out to disable GPS)
// -----------------------------------------------------------------------------
// #define GPS_RX_PIN  xx   // TODO: ESP32-S3 UART RX ← GP-02 TX
// #define GPS_TX_PIN  xx   // TODO: ESP32-S3 UART TX → GP-02 RX

// -----------------------------------------------------------------------------
// Optional control pins  (leave commented until physically wired)
// -----------------------------------------------------------------------------
// Power enable — active level must match GPS_EN_ACTIVE
// #define PIN_GPS_EN      xx
// #define GPS_EN_ACTIVE   HIGH

// Hard reset — active HIGH by default (GPS_RESET_MODE in GPS.cpp)
// #define PIN_GPS_RESET   xx

// Standby / force-on — active LOW by default (GPS_STANDBY_ACTIVE in GPS.cpp)
// #define PIN_GPS_STANDBY xx

// Pulse-per-second output from GP-02
// #define PIN_GPS_PPS     xx

// -----------------------------------------------------------------------------
// Board-specific GP-02 signals
// These have no Meshtastic framework support; implement in a variant.cpp if used.
// -----------------------------------------------------------------------------
// GP-02 VRTC    : external 3.3 V backup power enable (RTC memory retention)
// GP-02 FORCE_ON: wake from backup mode
// GP-02 GNSS_SEL: satellite constellation selection (GPS / GLONASS / BDS …)

// -----------------------------------------------------------------------------
// Baud rate
// GP-02 default is 9600 bps, which matches the Meshtastic framework default.
// Only define GPS_BAUDRATE if a different rate is needed.
// -----------------------------------------------------------------------------
// #define GPS_BAUDRATE  9600
