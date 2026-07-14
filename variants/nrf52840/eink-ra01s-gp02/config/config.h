// =============================================================================
// config/config.h — Software Configuration Aggregator
// =============================================================================
// Included from variant.h.  All software-level settings, module feature
// exclusions, calibration constants, and UI setup live in individual files
// under config/, sourced here in dependency-safe order.
// =============================================================================

#pragma once

#include "features.h"     // Module feature exclusions (MESHTASTIC_EXCLUDE_*)
#include "led.h"          // LED role → Meshtastic name mapping
#include "btn.h"          // Button role → Meshtastic name mapping
#include "lora.h"         // LoRa radio driver selection & tuning
#include "gps.h"          // GNSS baud rate & optional overrides
#include "display.h"      // E-Ink panel model, rotation, dynamic refresh
#include "battery.h"      // ADC calibration, OCV curve, divider ratio
#include "rtc.h"          // PCF8563 I2C address
// note: nicheGraphics.h is NOT included here — it is included directly by
// src/main.cpp via `#include "nicheGraphics.h"`.  Adding it to this chain
// would pull configuration.h (and its APP_VERSION dependency) into every
// Arduino core / library compilation unit, breaking the build.
#include "oem_splash.h"   // OEM boot logo bitmap & text
// anim_frames.h is NOT included here — it is an auto-generated animation data
// file meant to be included explicitly by renderer code when needed.
#include "test_config.h"  // Bring-up / power-measurement / debug helpers
