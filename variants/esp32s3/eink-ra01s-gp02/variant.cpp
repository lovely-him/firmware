// =============================================================================
// eink-ra01s-gp02  variant.cpp
// =============================================================================
// Board-specific hardware initialization hooks called by the Meshtastic
// framework and the Arduino core at startup.
//
// Available hooks (implement only what you need):
//
//   earlyInitVariant()  — Called very early, before most subsystems are up.
//                         Use for: deassert conflicting CS lines, power-on
//                         sequencing, SPI bus contention prevention.
//
//   initVariant()       — Called by the Arduino framework during setup().
//                         Use for: general GPIO init, peripheral power enable.
//                         Note: Arduino logging (LOG_*) may not be ready yet;
//                         use ESP_LOGI(TAG, ...) instead.
//
//   lateInitVariant()   — Called by Meshtastic after all subsystems are up.
//                         Use for: touch controllers, secondary displays,
//                         anything that depends on I2C/SPI being initialised.
//
// None of these are required — delete any that remain empty.
// =============================================================================

#include "variant.h"
#include "configuration.h"
#include <Arduino.h>

#include "esp_log.h"

static const char *TAG = "eink-ra01s-gp02";

// -----------------------------------------------------------------------------
// earlyInitVariant
// Called before SPI / I2C / radio subsystems are initialized.
// Use this to prevent bus contention by deassering all CS lines early.
// -----------------------------------------------------------------------------
// void earlyInitVariant()
// {
//     // Example: keep LoRa CS high so the SPI bus is not driven unexpectedly
//     // pinMode(LORA_CS, OUTPUT);
//     // digitalWrite(LORA_CS, HIGH);
// }

// -----------------------------------------------------------------------------
// initVariant
// Called by the Arduino core during setup(). GPIO init goes here.
// -----------------------------------------------------------------------------
void initVariant()
{
    ESP_LOGI(TAG, "Initializing eink-ra01s-gp02 variant");

    // TODO: add GPIO initialization as pins are wired up, e.g.:
    //
    // -- GPS power enable (if PIN_GPS_EN is defined and wired) --
    // pinMode(PIN_GPS_EN, OUTPUT);
    // digitalWrite(PIN_GPS_EN, GPS_EN_ACTIVE);
    //
    // -- GP-02 GNSS_SEL constellation selection pin --
    // pinMode(GNSS_SEL_PIN, OUTPUT);
    // digitalWrite(GNSS_SEL_PIN, LOW);   // LOW = GPS+GLONASS (board-specific)
    //
    // -- GP-02 FORCE_ON pin --
    // pinMode(GP02_FORCE_ON_PIN, OUTPUT);
    // digitalWrite(GP02_FORCE_ON_PIN, HIGH);
}

// -----------------------------------------------------------------------------
// lateInitVariant
// Called by Meshtastic after all subsystems (radio, GPS, display) are up.
// Use for peripherals that depend on I2C or SPI being already initialized.
// -----------------------------------------------------------------------------
// void lateInitVariant()
// {
//     // TODO: touch controller, secondary sensor init, etc.
// }
