// =============================================================================
// test_config.h — Temporary test and verification macro definitions
// (power measurement, functional validation, bring-up probes, etc.)
// All macros default to disabled (0). Enable individually when needed.
// Keeping these here avoids polluting the main variant.h configuration.
// =============================================================================

#pragma once

// -----------------------------------------------------------------------------
// POWER_TEST_SLEEP_AFTER_BLE / POWER_TEST_MEASURE_WINDOW_MS
// BLE power measurement window:
//   After POWER_TEST_SLEEP_AFTER_BLE ms (system settle time), all OSThreads are
//   suspended and the CPU enters FreeRTOS tickless idle. BLE advertising continues
//   in SoftDevice hardware (no thread required). Measure current with an oscilloscope
//   or ammeter during the POWER_TEST_MEASURE_WINDOW_MS window. Threads resume
//   automatically when the window expires.
// Target baseline: ~79 µA with JTAG connected (reference: NCS test #3).
// Remove both macros (and their usage in main.cpp) once the target is confirmed.
// -----------------------------------------------------------------------------
// #define POWER_TEST_SLEEP_AFTER_BLE   (60000)  // settle time before measurement window (ms)
// #define POWER_TEST_MEASURE_WINDOW_MS (30000)  // measurement window duration (ms); threads auto-resume after this

// -----------------------------------------------------------------------------
// TEST_FREEZE_AT_BOOT
// Freezes execution at the very start of setup() with an infinite loop, before
// any peripheral initialisation. Used to measure the hardware baseline current
// after SoftDevice init but before all other subsystems start.
// Location in main.cpp: entry of setup(), before powerHAL_init().
// -----------------------------------------------------------------------------
#define TEST_FREEZE_AT_BOOT 0

// -----------------------------------------------------------------------------
// TEST_PLAN_B_BLE_ADV
// Minimal BLE advertising test: bypasses full Meshtastic initialisation and only
// starts Bluefruit BLE advertising.
//   Phase 1 (0–30 s): advertise at BLE_ADV_FAST_INTERVAL / SLOW_INTERVAL.
//   Phase 2 (30 s+):  stop advertising and enter infinite low-power idle
//                     (expected baseline ~34 µA).
// Do NOT measure while powered via USB — USB keeps loopCanSleep() false,
// which prevents tickless sleep.
// Reference: NCS test #3, 79.63 µA, interval 800–1600 × 0.625 ms.
// -----------------------------------------------------------------------------
#define TEST_PLAN_B_BLE_ADV 0

// -----------------------------------------------------------------------------
// POWER_THROTTLE_MS
// Cancel+Enter combo key: toggles thread-throttle mode at runtime.
//   ON : all OSThreads with interval < POWER_THROTTLE_MS are set to POWER_THROTTLE_MS.
//        Threads already sleeping longer are left unchanged.
//   OFF: all threads are called setIntervalFromNow(0) so they self-reschedule
//        immediately and resume normal operation.
// Tweak this value to balance power vs. responsiveness.
// -----------------------------------------------------------------------------
#define POWER_THROTTLE_MS INT32_MAX // (ps:5000UL)

// -----------------------------------------------------------------------------
// SHUTDOWN_HALT_STEP
// Freeze the device in an infinite loop after the Nth step of the shutdown
// sequence. Allows measuring static current at each stage with an ammeter.
// Trigger shutdown normally (Fn long-press 4s), then measure.
// Watchdog resets the device after ~90s automatically.
//
//  0  disabled — normal shutdown (default)
//  1  after waitEnterSleep()                — BT advertising stopped (1st call)
//  2  after notifyDeepSleep observers       — LoRa sleep + GPS LDO off
//  3  after screen->doDeepSleep()           — eInk LDO off
//  4  after Wire.end()                      — I2C TWIM disabled
//  5  after SPI.end() + SPI1.end()          — LoRa + eInk SPI buses disabled
//  6  after Serial.end() + Serial1.end()    — USB CDC + GPS UART disabled
//  7  after setBluetoothEnable(false) ×2    — BT shutdown (2nd call)
//  8  after variant_shutdown()              — all pins tristated, LDOs cut
// -----------------------------------------------------------------------------
// #define SHUTDOWN_HALT_STEP 8

// Internal helper — do not call directly.
// Compiles to nothing when SHUTDOWN_HALT_STEP == 0 or board is not EINK_RA01S_GP02.
#if defined(EINK_RA01S_GP02) && defined(SHUTDOWN_HALT_STEP)
#define SHUTDOWN_HALT_AT(N)                                                                                                      \
    do {                                                                                                                         \
        if (SHUTDOWN_HALT_STEP == (N)) {                                                                                         \
            while (1) {                                                                                                          \
                delay(60000);                                                                                                    \
                LOG_INFO("SHUTDOWN_HALT_STEP %d: device is halted in an infinite loop for measurement. Watchdog will reset.",    \
                         (N));                                                                                                   \
            }                                                                                                                    \
        }                                                                                                                        \
    } while (0)
#endif
