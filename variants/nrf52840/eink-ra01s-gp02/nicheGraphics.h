#pragma once

#include "configuration.h"

#ifdef MESHTASTIC_INCLUDE_NICHE_GRAPHICS

// =============================================================================
// eink-ra01s-gp02 InkHUD setup
//
// Input is handled entirely by SixKeyNavInput (initialized in InputBroker.cpp):
//   Up / Down / Left / Right → inkhud->navUp/Down/Left/Right()
//   Enter                    → inkhud->shortpress()
//   Fn + nav                 → Fn combo callbacks (BLE/GPS/LoRa toggle, etc.)
//   Fn held ≥ 4 s            → shutdown
//
// No TwoButton / TwoButtonExtended is created here.
// =============================================================================

// InkHUD core
#include "graphics/niche/InkHUD/InkHUD.h"
// NavCombos power management (setInkHudDriver for wakeup reinit)
#include "input/NavCombos.h"

// Applets
#include "graphics/niche/InkHUD/Applets/User/AllMessage/AllMessageApplet.h"
#include "graphics/niche/InkHUD/Applets/User/DM/DMApplet.h"
#include "graphics/niche/InkHUD/Applets/User/FavoritesMap/FavoritesMapApplet.h"
#include "graphics/niche/InkHUD/Applets/User/Heard/HeardApplet.h"
#include "graphics/niche/InkHUD/Applets/User/Positions/PositionsApplet.h"
#include "graphics/niche/InkHUD/Applets/User/RecentsList/RecentsListApplet.h"
#include "graphics/niche/InkHUD/Applets/User/ThreadedMessage/ThreadedMessageApplet.h"

// E-Ink driver — GDEY0213B74 (SSD16XX family, 2.13", 250×122)
// Compatible with our GDEM0213B74 (same SSD1680 controller, same dimensions).
#include "graphics/niche/Drivers/EInk/GDEY0213B74.h"

void setupNicheGraphics()
{
    using namespace NicheGraphics;

    // -------------------------------------------------------------------------
    // Panel power
    // PIN_EINK_EN (P1.12) controls the display power rail.
    // Must be HIGH before SPI1.begin() / driver->begin().
    // -------------------------------------------------------------------------
    pinMode(PIN_EINK_EN, OUTPUT);
    digitalWrite(PIN_EINK_EN, HIGH);

    // -------------------------------------------------------------------------
    // SPI1 — dedicated E-Ink bus (SCLK=P0.31, MOSI=P0.29, CS=P0.30)
    // -------------------------------------------------------------------------
    SPI1.begin();

    // -------------------------------------------------------------------------
    // E-Ink driver
    // -------------------------------------------------------------------------
    Drivers::EInk *driver = new Drivers::GDEY0213B74;
    driver->begin(&SPI1, PIN_EINK_DC, PIN_EINK_CS, PIN_EINK_BUSY, PIN_EINK_RES);

    // -------------------------------------------------------------------------
    // InkHUD
    // -------------------------------------------------------------------------
    InkHUD::InkHUD *inkhud = InkHUD::InkHUD::getInstance();

    // Register driver + SPI with NavCombos so Fn+Down can power-off the
    // display panel and any subsequent keypress can reinitialize it.
    setInkHudDriver(driver, &SPI1);

    inkhud->setDriver(driver);

    // FAST:FULL refresh ratio and stress multiplier
    inkhud->setDisplayResilience(20, 1.5);

    // Fonts
    InkHUD::Applet::fontLarge = FREESANS_12PT_WIN1252;
    InkHUD::Applet::fontMedium = FREESANS_9PT_WIN1252;
    InkHUD::Applet::fontSmall = FREESANS_6PT_WIN1252;

    // Default settings
    // rotation=3 → 270° clockwise → landscape 250×122, matching EINK_ROTATION=3
    // joystick.alignment=1 → cancels rotation: (3+1)%4=0 → physical Up/Down/
    //   Left/Right map 1:1 to display Up/Down/Left/Right without axis swapping.
    inkhud->persistence->settings.rotation = 3;
    inkhud->persistence->settings.joystick.enabled = true;
    inkhud->persistence->settings.joystick.alignment = 1;
    inkhud->persistence->settings.optionalMenuItems.nextTile = false; // use Left/Right keys
    inkhud->persistence->settings.userTiles.maxCount = 2;
    inkhud->persistence->settings.optionalFeatures.batteryIcon = true;

    // -------------------------------------------------------------------------
    // Applets (order determines auto-show priority)
    // -------------------------------------------------------------------------
    inkhud->addApplet("All Messages", new InkHUD::AllMessageApplet, true, true); // activated, autoshown
    inkhud->addApplet("DMs", new InkHUD::DMApplet);
    inkhud->addApplet("Channel 0", new InkHUD::ThreadedMessageApplet(0));
    inkhud->addApplet("Channel 1", new InkHUD::ThreadedMessageApplet(1));
    inkhud->addApplet("Positions", new InkHUD::PositionsApplet, true);
    inkhud->addApplet("Recents", new InkHUD::RecentsListApplet);
    inkhud->addApplet("Heard", new InkHUD::HeardApplet, true, false, 0);
    inkhud->addApplet("Favorites Map", new InkHUD::FavoritesMapApplet);

    inkhud->begin();

    // -------------------------------------------------------------------------
    // Input: SixKeyNavInput is already running (InputBroker.cpp → Init()).
    // In EINK_RA01S_GP02_INKHUD mode, SixKeyNavInput calls InkHUD API directly
    // for nav keys; Fn combos still route through InputBroker as before.
    // No TwoButton setup needed here.
    // -------------------------------------------------------------------------
}

#endif // MESHTASTIC_INCLUDE_NICHE_GRAPHICS
