#pragma once
#include <stdint.h> // uint8_t — satisfies IntelliSense; already provided by Arduino at compile time
// =============================================================================
// Battery ADC — eink-ra01s-gp02
// =============================================================================
// Hardware:
//   ADC input  : P0.04 (AIN2, Arduino pin 4) — lower end of voltage divider
//   Power ctrl : P1.04 (Arduino pin 36) — PMOS gate, active-low
//                LOW  → PMOS ON  → divider powered  → valid reading
//                HIGH → PMOS OFF → divider floating  → zero quiescent current
//   Divider    : R1 = R2 = 2 kΩ  →  V_adc = V_batt × 0.5  →  ADC_MULTIPLIER = 2.0
//   Resolution : 14-bit SAADC
//   Reference  : VDD = 3.3 V (Normal Voltage mode, no external AREF)
//
// Meshtastic power path:
//   Power.cpp battery_adcEnable()  → digitalWrite(ADC_CTRL, LOW)  before burst
//   Power.cpp battery_adcDisable() → digitalWrite(ADC_CTRL, HIGH) after  burst
//   AnalogBatteryLevel::getBattVoltage():
//     raw   = average of BATTERY_SENSE_SAMPLES analogRead(BATTERY_PIN) calls
//     V_mV  = ADC_MULTIPLIER × (1000 × AREF_VOLTAGE / 2^14) × raw
//           = 2.0 × (1000 × 3.3 / 16384) × raw  ≈ raw × 0.4028 mV/LSB
//   AnalogBatteryLevel::isBatteryConnect():
//     returns getBatteryPercent() != -1
//     getBatteryPercent() returns -1 when V_mV < noBatVolt (2600 mV)
//     → no battery connected (or flat dead) → Meshtastic treats as "external power"
//     → USE_POWERSAVE in variant.h overrides this during no-battery bench testing
// =============================================================================

// -----------------------------------------------------------------------------
// ADC input pin
// -----------------------------------------------------------------------------
#define BATTERY_PIN 4                    // P0.04 = AIN2 (Arduino pin 4)
#define ADC_MULTIPLIER 2.0f              // 50:50 divider: V_batt = V_adc × 2
#define BATTERY_SENSE_RESOLUTION_BITS 14 // nRF52 SAADC 14-bit mode
static const uint8_t A0 = BATTERY_PIN;
#define ADC_RESOLUTION 14

// -----------------------------------------------------------------------------
// Voltage-divider power-enable (PMOS gate, active-low)
// P1.04 = Arduino pin 36 (32 + 4).
// Power.cpp guards every analogRead() burst with adcEnable/adcDisable so the
// 2 kΩ divider draws current only during measurement (~1 ms per sample burst).
// -----------------------------------------------------------------------------
#define ADC_CTRL (32 + 4)    // P1.04 — PMOS gate
#define ADC_CTRL_ENABLED LOW // LOW = PMOS ON = divider energised

// -----------------------------------------------------------------------------
// ADC reference voltage
// Adafruit nRF52 Arduino BSP configures SAADC with internal 0.6 V reference
// and 1/6 gain → full-scale = 0.6 V / (1/6) = 3.6 V  (NOT VDD = 3.3 V).
// Using 3.3 causes a systematic 9 % under-read:
//   measured 3071 mV × (3.6/3.3) = 3349 mV ≈ actual 3.3 V source.
// AREF_VOLTAGE is intentionally NOT defaulted for ARCH_NRF52 in Power.cpp:
//   #if !defined(AREF_VOLTAGE) && !defined(ARCH_NRF52)
// Each nRF52 variant must declare it here; omitting it causes a compile error.
// Reference: seeed_wio_tracker_L1_eink (nRF52840 + E-ink) also uses 3.6.
// -----------------------------------------------------------------------------
#define AREF_VOLTAGE 3.6

// -----------------------------------------------------------------------------
// OCV (Open-Circuit Voltage) discharge curve — standard LiPo single cell
//
// Values in millivolts, ordered 100 % SOC → 0 % SOC (11 equally-spaced points).
// Replace the OCV_ARRAY with the manufacturer datasheet curve once the actual
// battery model is confirmed.
//
// Thresholds derived automatically by Power.cpp / AnalogBatteryLevel:
//   chargingVolt = OCV[0]  + 10  = 4200 mV  → isVbusIn() = true  (charging detected)
//   noBatVolt    = OCV[10] - 500 = 2600 mV  → isBatteryConnect() = false (no battery)
//   low_batt_sds = OCV[10]       = 3100 mV  → EVENT_LOW_BATTERY after 10 consecutive
//                                              readings below this → doDeepSleep()
// -----------------------------------------------------------------------------
#define NUM_OCV_POINTS 11
#define OCV_ARRAY 4190, 4050, 3990, 3890, 3800, 3720, 3630, 3530, 3420, 3300, 3100
#define NUM_CELLS 1
