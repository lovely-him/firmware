// =============================================================================
// eink-ra01s-gp02  nRF52840 Board Configuration
// =============================================================================
// Hardware:
//   MCU     : nRF52840 QFN48 (Normal Voltage mode: VDD=VDDH=3.3V, LDO only)
//   LoRa    : Ai-Thinker RA-01S (SX1262/SX1268, 868/915 MHz)  → lora_sx1262.h
//   GPS     : Ai-Thinker GP-02  (L76K compatible)              → gps_gp02.h
//   E-Ink   : GDEM0213B74 (SSD1680, 2.13", 250×122)           → display_eink.h
//   Buttons : 6-way nav (Enter/L/R/U/D/Fn) + 3 LEDs (R/G/B)  → peripherals.h
//   RTC     : PCF8563 on I2C (P0.26 SDA / P0.27 SCL)          → peripherals.h
//   Battery : ADC on P0.04 (AIN2), power-ctrl P1.04            → battery_adc.h
//   BT      : S140 v6.1.1 SoftDevice (app @ 0x26000)
// =============================================================================

#ifndef _VARIANT_EINK_RA01S_GP02_NRF52840_
#define _VARIANT_EINK_RA01S_GP02_NRF52840_

// -----------------------------------------------------------------------------
// Hardware identity
// PRIVATE_HW is defined via -D PRIVATE_HW in platformio.ini build_flags.
// → meshtastic_HardwareModel_PRIVATE_HW (value 255)
// architecture.h checks: #elif defined(PRIVATE_HW) || defined(FEATHER_DIY)
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Master clock (HF crystal, not configurable)
// -----------------------------------------------------------------------------
#define VARIANT_MCK (64000000ul)

// -----------------------------------------------------------------------------
// Low-frequency clock source
// Board has 32.768kHz external crystal → USE_LFXO.
// wiring.c reads this macro and sets NRF_CLOCK->LFCLKSRC accordingly.
// WARNING: Must define exactly one of USE_LFXO or USE_LFRC, or build fails.
// -----------------------------------------------------------------------------
#define USE_LFXO // Board has 32.768kHz crystal for LF clock

// -----------------------------------------------------------------------------
// DC/DC regulator
// Board uses Normal Voltage mode (VDD shorted to VDDH, 3.3V supply).
// In Normal Voltage mode REG0 is automatically disabled by hardware.
// REG1 operates in LDO mode (no external inductor present on this board).
// DO NOT enable DCDC (sd_power_dcdc_mode_set) without an external LC filter —
// this will lock out the device including SWD debug access.
// initVariant() intentionally does NOT call sd_power_dcdc_mode_set().
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// WVariant.h required
// -----------------------------------------------------------------------------
#include "WVariant.h"

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Pin count
// nRF52840 has two ports: P0 (pins 0–31) and P1 (pins 32–47).
// -----------------------------------------------------------------------------
#define PINS_COUNT (48)
#define NUM_DIGITAL_PINS (48)
#define NUM_ANALOG_INPUTS (6)
#define NUM_ANALOG_OUTPUTS (0)

// =============================================================================
// Hardware feature flags
// =============================================================================
#define HAS_RADIO 1  // LoRa:  RA-01S (SX1262), pins in lora_sx1262.h
#define HAS_GPS 1    // GPS:   GP-02 (L76K), UART P1.08/P1.09, LDO P0.12, RST P1.05, PPS P1.04
#define HAS_SCREEN 1 // E-Ink: GDEM0213B74 (SSD1680), pins in display_eink.h

// -----------------------------------------------------------------------------
// SerialModule uses Serial2 (ESP32) by default. nRF52840 must override to Serial (USB).
// All other nRF52840 variants define this; omitting it causes a compile error on nRF52.
// -----------------------------------------------------------------------------
#define SERIAL_PRINT_PORT 0

// -----------------------------------------------------------------------------
// NRF_APM: use nrfx_power_usbstatus_get() to detect USB VBUS state.
// Required for correct isPowered() / USB detection on nRF52840 without external PMU.
// Enables the NRF_APM code block in Power.cpp (readPowerStatus).
// -----------------------------------------------------------------------------
#define NRF_APM

// -----------------------------------------------------------------------------
// USE_POWERSAVE: forces config.power.is_power_saving = true, making isPowered()
// always return false → PowerFSM enters stateON → canSleep = true → FreeRTOS
// tickless idle. Enable when bench-testing without a battery: ADC reads ~0 mV
// → isBatteryConnect() = false → Meshtastic assumes external power → canSleep = false.
// Battery is connected on this board; BATTERY_PIN + NRF_APM handle detection correctly.
// -----------------------------------------------------------------------------
// #define USE_POWERSAVE

// -----------------------------------------------------------------------------
// USE_SEGGER
// Redirects LOG_* output to SEGGER RTT (via SWD/J-Link) instead of USB-CDC.
// Use when the device has no USB connected (view logs via JLinkRTTViewer or
// telnet localhost:19021). Prevents USB connection from keeping loopCanSleep()
// returning false, which would block FreeRTOS tickless idle.
// To disable: comment out this line (value 0 is not checked by the framework).
// -----------------------------------------------------------------------------
// #define USE_SEGGER

// -----------------------------------------------------------------------------
// BLE fixed pairing PIN
// USERPREFS_FIXED_BLUETOOTH: forces PairingMode_FIXED_PIN with the given value.
// Without this, boards with a screen default to RANDOM_PIN (shown on display).
// Defined here to keep a known PIN independent of pairing-mode defaults.
// Value must be a numeric string (e.g. "123456"), converted to uint32 by NodeDB.
// -----------------------------------------------------------------------------
#define USERPREFS_FIXED_BLUETOOTH 123456

// -----------------------------------------------------------------------------
// BLE advertising interval
// Unit: 0.625 ms (SoftDevice S140 BLE_GAP_ADV_INTERVAL units)
//   BLE_ADV_FAST_INTERVAL = 800  → 800 × 0.625 =  500 ms
//   BLE_ADV_SLOW_INTERVAL = 1600 → 1600 × 0.625 = 1000 ms
//   BLE_ADV_FAST_TIMEOUT_S = 0   → skip fast mode, use slow interval immediately
// Reference: NCS test baseline at 800–1600 × 0.625 ms (same values)
// -----------------------------------------------------------------------------
#define BLE_ADV_FAST_INTERVAL 800
#define BLE_ADV_SLOW_INTERVAL 1600
#define BLE_ADV_FAST_TIMEOUT_S 0

// =============================================================================
// Feature exclusions
// Checked by configuration.h before any module is compiled.
// =============================================================================

// SixKeyNavInput: 6-button polling driver (Enter/L/R/U/D/Fn), activated by -D EINK_RA01S_GP02
#define MESHTASTIC_EXCLUDE_SIXKEYNAVINPUT 0

// I2C active: PCF8563 RTC on P0.26 (SDA) / P0.27 (SCL), pull-ups on PCB.
// ScanI2CTwoWire detects PCF8563 at 0x51 via PCF8563_RTC hint.
#define MESHTASTIC_EXCLUDE_I2C 0

// PowerFSM required: setBluetoothEnable(true) is called from PowerFSM state handlers.
// Without it, BLE advertising never starts.
#define MESHTASTIC_EXCLUDE_POWER_FSM 0 // DO NOT exclude: kills BLE

// No external sensors or motion hardware
#define MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR 1
#define MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR_EXTERNAL 1
#define MESHTASTIC_EXCLUDE_POWER_TELEMETRY 1
#define MESHTASTIC_EXCLUDE_AIR_QUALITY_SENSOR 1
#define MESHTASTIC_EXCLUDE_HEALTH_TELEMETRY 1
#define MESHTASTIC_EXCLUDE_ACCELEROMETER 1
#define MESHTASTIC_EXCLUDE_DETECTIONSENSOR 1

// No audio (nrf52_base already sets this; repeated for clarity)
#define MESHTASTIC_EXCLUDE_AUDIO 1

// No BLE PAX counting (nrf52_base already sets this)
#define MESHTASTIC_EXCLUDE_PAXCOUNTER 1

// No peripheral notification module
#define MESHTASTIC_EXCLUDE_EXTERNALNOTIFICATION 1

// Unused features and network modules
#define MESHTASTIC_EXCLUDE_MQTT 1           // nRF52 has no WiFi
#define MESHTASTIC_EXCLUDE_SERIAL 1         // Serial bridge module, not needed
#define MESHTASTIC_EXCLUDE_STOREFORWARD 1   // RAM-heavy store-and-forward, not needed
#define MESHTASTIC_EXCLUDE_DROPZONE 1       // File sharing, not needed
#define MESHTASTIC_EXCLUDE_ATAK 1           // ATAK integration, not needed
#define MESHTASTIC_EXCLUDE_REPLYBOT 1       // Auto-reply bot, not needed
#define MESHTASTIC_EXCLUDE_REMOTEHARDWARE 1 // Remote GPIO control, not needed
#define MESHTASTIC_EXCLUDE_POWERSTRESS 1    // Stress-test tool, not for production

// =============================================================================
// OEM boot screen (logo image + text shown after Meshtastic splash)
// =============================================================================
#include "oem_splash.h"

// =============================================================================
// Test / verification macro overrides
// =============================================================================
#include "test_config.h"

// =============================================================================
// Sub-module pin definitions
// =============================================================================
#include "battery_adc.h"
#include "display_eink.h"
#include "gps_gp02.h"
#include "lora_sx1262.h"
#include "peripherals.h"

#ifdef __cplusplus
}
#endif

#endif // _VARIANT_EINK_RA01S_GP02_NRF52840_
