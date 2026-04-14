// =============================================================================
// eink-ra01s-gp02  ESP32-S3 Board Configuration
// =============================================================================
// Hardware:
//   MCU  : ESP32-S3-WROOM-1-N16R8 (16MB Flash, 8MB Octal PSRAM)
//   LoRa : Ai-Thinker RA-01S (SX1262, no TCXO, external crystal)
//   GPS  : Ai-Thinker GP-02 (NMEA 9600 bps, L76K compatible)
//   EInk : TODO
// =============================================================================

#pragma once

// -----------------------------------------------------------------------------
// Hardware vendor identification
// Required: configuration.h checks HW_VENDOR via architecture.h.
// PRIVATE_HW maps to meshtastic_HardwareModel_PRIVATE_HW.
// -----------------------------------------------------------------------------
#define PRIVATE_HW

// -----------------------------------------------------------------------------
// Feature toggles
// architecture.h defaults for ESP32: HAS_SCREEN=1, HAS_GPS=1, HAS_BUTTON=1,
// HAS_WIFI=1, HAS_BLUETOOTH=1, HAS_TELEMETRY=1, HAS_SENSOR=1, HAS_RADIO=1.
// Anything not explicitly set to 0 here remains enabled by architecture.h.
// -----------------------------------------------------------------------------
#define HAS_RADIO                                                                                                                \
    0                // LoRa temporarily disabled — pins not yet wired, testing BLE only
                     // NOTE: lora_sx1262.h is still included below to keep all LORA_* pins at -1.
                     // Without it, architecture.h would substitute ESP32-classic defaults
                     // (GPIO 5/18/19/27) which trample ESP32-S3 qio_opi PSRAM pins → TG1WDT.
#define HAS_SCREEN 0 // No display connected yet
#define HAS_GPS 0    // GPS disabled until pins are filled in
#define HAS_BUTTON 0 // No user button defined yet
#define HAS_SENSOR 0 // No external sensors wired (also covered by EXCLUDE macros below)
// HAS_WIFI      : do NOT set here — MESHTASTIC_EXCLUDE_WIFI below forces HAS_WIFI=0 automatically
// HAS_BLUETOOTH : leave enabled (ESP32-S3 built-in)
// HAS_TELEMETRY : leave enabled (broadcasts node health: battery, air util, channel util)
// HAS_RADIO     : leave enabled (LoRa required for mesh)

// Power override: board has no PMU and no BATTERY_PIN ADC.
// Without this, isPowered() assumes external power → FSM stuck in statePOWER → canSleep=false.
#define DEVICE_HAS_BATTERY // assume battery-powered, override isPowered()=false

// Power saving: screen_on_secs=30, wait_bluetooth_secs=30, is_power_saving=true
// Enables FSM path: ON →(30s)→ DARK →(30s)→ LS (light sleep)
// #define USE_POWERSAVE

// -----------------------------------------------------------------------------
// Module exclusions
// Defined here (not in platformio.ini) so that IDE jump-to-definition works.
// configuration.h includes variant.h at line 37, before all MESHTASTIC_EXCLUDE_*
// checks, so these macros are visible to every compilation unit.
// -----------------------------------------------------------------------------
#define MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR 1 // no BMP/BME/DPS/MCP/INA/etc
#define MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR_EXTERNAL 1
#define MESHTASTIC_EXCLUDE_POWER_TELEMETRY 1    // no MAX17048/INA power monitor
#define MESHTASTIC_EXCLUDE_AIR_QUALITY_SENSOR 1 // no SGP/SEN5x
#define MESHTASTIC_EXCLUDE_HEALTH_TELEMETRY 1   // no MAX30102
#define MESHTASTIC_EXCLUDE_ACCELEROMETER 1      // no IMU/accelerometer
#define MESHTASTIC_EXCLUDE_I2C                                                                                                   \
    1                                   // no I2C devices on this board;
                                        // also fixes MotionSensor.cpp compile
                                        // error when HAS_SCREEN=0 (config undeclared)
#define MESHTASTIC_EXCLUDE_AUDIO 1      // no microphone / codec2
#define MESHTASTIC_EXCLUDE_PAXCOUNTER 1 // no WiFi/BLE pax counting
#define MESHTASTIC_EXCLUDE_WIFI                                                                                                  \
    1                                             // no WiFi — power saving, BLE-only config
                                                  // also auto-triggers: EXCLUDE_WEBSERVER,
                                                  //   HAS_WIFI=0, HAS_NETWORKING=0
#define MESHTASTIC_EXCLUDE_MQTT 1                 // no MQTT (requires WiFi/networking anyway)
#define MESHTASTIC_EXCLUDE_EXTERNALNOTIFICATION 1 // no buzzer/LED notification
#define MESHTASTIC_EXCLUDE_RANGETEST 1            // no range test module
#define MESHTASTIC_EXCLUDE_SERIAL 1               // no serial module
#define MESHTASTIC_EXCLUDE_STOREFORWARD 1         // no store-and-forward
#define MESHTASTIC_EXCLUDE_DROPZONE 1             // no drop zone
#define MESHTASTIC_EXCLUDE_ATAK 1                 // no ATAK plugin
#define MESHTASTIC_EXCLUDE_REPLYBOT 1             // no reply bot
// #define MESHTASTIC_EXCLUDE_SCREEN               1 // uncomment when eink is NOT wired
// #define MESHTASTIC_EXCLUDE_GPS                  1 // uncomment if GPS is removed
// #define MESHTASTIC_EXCLUDE_BLUETOOTH            1 // uncomment to strip BLE stack (~200KB)

#include "display_eink.h" // E-Ink display configuration
#include "gps_gp02.h"     // GP-02 GPS configuration
#include "lora_sx1262.h"  // SX1262 LoRa radio configuration (pin defs only; HAS_RADIO=0 disables driver)
#include "peripherals.h"  // I2C, LED, NeoPixel, Battery ADC
