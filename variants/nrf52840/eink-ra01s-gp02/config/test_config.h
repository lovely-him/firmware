// =============================================================================
// config/test_config.h — Temporary Test & Verification Macros
// (power measurement, bring-up probes, functional validation, etc.)
// All macros default to disabled (0). Enable individually when needed.
// =============================================================================

#pragma once

// -----------------------------------------------------------------------------
// POWER_TEST_SLEEP_AFTER_BLE / POWER_TEST_MEASURE_WINDOW_MS
// BLE power measurement window.
// -----------------------------------------------------------------------------
// #define POWER_TEST_SLEEP_AFTER_BLE   (60000)
// #define POWER_TEST_MEASURE_WINDOW_MS (30000)

// -----------------------------------------------------------------------------
// TEST_FREEZE_AT_BOOT
// Freezes execution at the very start of setup() before any peripheral init.
// -----------------------------------------------------------------------------
#define TEST_FREEZE_AT_BOOT 0

// -----------------------------------------------------------------------------
// TEST_PLAN_B_BLE_ADV
// Minimal BLE advertising test.
// -----------------------------------------------------------------------------
#define TEST_PLAN_B_BLE_ADV 0

// -----------------------------------------------------------------------------
// POWER_THROTTLE_MS
// Cancel+Enter combo: toggle thread-throttle mode.
// -----------------------------------------------------------------------------
#define POWER_THROTTLE_MS INT32_MAX

// -----------------------------------------------------------------------------
// SHUTDOWN_HALT_STEP
// Freeze after N-th step of shutdown sequence for current measurement.
// -----------------------------------------------------------------------------
// #define SHUTDOWN_HALT_STEP 8

#if defined(EINK_RA01S_GP02) && defined(SHUTDOWN_HALT_STEP)
#define SHUTDOWN_HALT_AT(N)                                                                                                    \
    do {                                                                                                                       \
        if (SHUTDOWN_HALT_STEP == (N)) {                                                                                       \
            while (1) {                                                                                                        \
                delay(60000);                                                                                                  \
                LOG_INFO("SHUTDOWN_HALT_STEP %d: device is halted in an infinite loop for measurement. Watchdog will reset.",   \
                         (N));                                                                                                 \
            }                                                                                                                  \
        }                                                                                                                      \
    } while (0)
#endif
