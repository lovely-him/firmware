// =============================================================================
// config/display.h — E-Ink Display Configuration
// =============================================================================
// Panel model selection, orientation, and dynamic refresh tuning.
//
// NOTE: USE_EINK and USE_EINK_DYNAMICDISPLAY must be -D build flags in
// platformio.ini (the external GxEPD2 library uses them but never includes
// configuration.h).  All other EINK_* tuning macros are safe here.
// =============================================================================

#pragma once

// ---------------------------------------------------------------------------
// Panel model
// GxEPD2_213_B74: driver specifically for GDEM0213B74 (SSD1680).
// Uses the OTP built-in LUT for partial updates — correct for this panel.
// Physical: WIDTH=128 gate lines, HEIGHT=250 source lines.
// After setRotation(3): display coordinate width=250, height=128.
// ---------------------------------------------------------------------------
#define EINK_DISPLAY_MODEL  GxEPD2_213_B74
#define EINK_WIDTH          250   // display width  after rotation-3 (source lines)
#define EINK_HEIGHT         122   // display height after rotation-3 (visible gate lines)

// ---------------------------------------------------------------------------
// Screen rotation
//   0 → 250w × 122h  landscape, FPC at bottom
//   1 → 122w × 250h  portrait
//   2 → 250w × 122h  landscape, FPC at top (180°)
//   3 → 122w × 250h  portrait, rotated 180°
// ---------------------------------------------------------------------------
#define EINK_ROTATION   3

// ---------------------------------------------------------------------------
// Dynamic refresh control
// ---------------------------------------------------------------------------
#define EINK_BACKGROUND_USES_FAST
#define EINK_HASQUIRK_GHOSTING

// EINK_ASYNC_FAST: fast (partial) refreshes also run asynchronously on the
// worker thread — the firmware polls BUSY and then runs the post-refresh
// write-back (writeImageAgain) + hibernate, so the UI thread is not blocked
// while the panel refreshes. Ghosting is handled by the standard
// writeImage → refresh → writeImageAgain protocol (no repeat-refresh needed).
#define EINK_ASYNC_FAST

// Old "force full refresh" fallbacks — intentionally disabled.
// The ghosting-repeat mechanism replaces both. Uncomment to restore.
// #define EINK_ALLOW_FORCE_FULL               // FULL after EINK_LIMIT_FASTREFRESH consecutive fast refreshes
// #define EINK_ALLOW_GHOST_FALLBACK           // FULL when ghost-pixel count exceeds limit
#ifdef EINK_ALLOW_FORCE_FULL
#define EINK_LIMIT_FASTREFRESH  (20)
#endif
#ifdef EINK_ALLOW_GHOST_FALLBACK
#define EINK_LIMIT_GHOSTING_PX  (EINK_WIDTH * EINK_HEIGHT / 2)
#endif
