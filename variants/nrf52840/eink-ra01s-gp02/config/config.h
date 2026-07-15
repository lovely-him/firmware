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
