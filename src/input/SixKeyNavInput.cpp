#include "SixKeyNavInput.h"

#if defined(EINK_RA01S_GP02)

#include "configuration.h"
#include <nrf_gpio.h> // nrf_gpio_cfg_sense_input (HAL, header-only)

#ifdef EINK_RA01S_GP02_INKHUD
#include "graphics/niche/InkHUD/InkHUD.h"
#include "input/NavCombos.h" // inkHudIsDisplayOff(), inkHudWakeup()
#endif

SixKeyNavInput *sixKeyNavInput = nullptr;

// File-local singleton pointer used by the static ISR.
static SixKeyNavInput *s_instance = nullptr;

// ---------------------------------------------------------------------------
// Navigation button table: Up / Down / Left / Right / Enter
// Indexed 0-4 to match _nav[].
// ---------------------------------------------------------------------------
static const struct {
    uint8_t pin;
    input_broker_event evt;
    const char *name;
} NAV_TABLE[5] = {
    {PIN_BUTTON_UP, INPUT_BROKER_UP, "Up"},           {PIN_BUTTON_DOWN, INPUT_BROKER_DOWN, "Down"},
    {PIN_BUTTON_LEFT, INPUT_BROKER_LEFT, "Left"},     {PIN_BUTTON_RIGHT, INPUT_BROKER_RIGHT, "Right"},
    {PIN_BUTTON_ENTER, INPUT_BROKER_SELECT, "Enter"},
};

// ---------------------------------------------------------------------------
SixKeyNavInput::SixKeyNavInput() : concurrency::OSThread("SixKeyNav")
{
    _originName = "SixKeyNav";
    _fnConsumed = false;
    _fnPressTime = 0;
    _navSnapshot = 0;
    _navPressedMask = 0;
    _fnSnapshot = false;
    _snapshotReady = false;
    onComboUp = nullptr;
    onComboDown = nullptr;
    onComboLeft = nullptr;
    onComboRight = nullptr;
    onComboEnter = nullptr;
    onFnHeld = nullptr;
}

// Forward-declare the GpioteOverride API (defined in GpioteOverride.cpp).
extern "C" {
void registerGpiotePortPin(uint32_t physPin);
void enableGpiotePortCallback(void (*cb)(void));
}

// ---------------------------------------------------------------------------
// Shared PORT-event callback for all 6 button pins.
// Called from GPIOTE_IRQHandler (GpioteOverride.cpp) on any button edge.
//
// Reads all 6 pin levels immediately (active-low: LOW → pressed = bit set),
// stores them as a snapshot, then wakes the OSThread for processing.
// If the thread hasn't consumed the previous snapshot yet, the new one
// simply overwrites it - "keep latest, process once" policy.
// ---------------------------------------------------------------------------
static void gpiotePortHandler()
{
    if (!s_instance)
        return;
    s_instance->captureSnapshotFromISR();
    s_instance->setIntervalFromNow(0);
    runASAP = true;
    BaseType_t higherWake = 0;
    concurrency::mainDelay.interruptFromISR(&higherWake);
}

// ---------------------------------------------------------------------------
// Called from the ISR (gpiotePortHandler) with interrupts masked.
// Reads all 6 pin levels and stores them as a volatile snapshot.
// New snapshot overwrites any unconsumed previous one ("keep latest" policy).
// ---------------------------------------------------------------------------
void SixKeyNavInput::captureSnapshotFromISR()
{
    uint8_t navSnap = 0;
    for (int i = 0; i < 5; i++) {
        if (!digitalRead(_nav[i].pin))
            navSnap |= (1u << i);
    }
    _navSnapshot = navSnap;
    _navPressedMask |= navSnap; // sticky: accumulate presses; thread clears
    _fnSnapshot = !digitalRead(_fn.pin);
    _snapshotReady = true;
}

// ---------------------------------------------------------------------------
bool SixKeyNavInput::init()
{
    s_instance = this;

    // Configure all 6 pins for GPIOTE PORT event via GPIO SENSE/DETECT:
    //   registerGpiotePortPin() calls nrf_gpio_cfg_sense_input(PULLUP, SENSE_LOW)
    //   which routes through the shared PORT event - zero dedicated GPIOTE channels.
    //   System ON sleep current: ~2.4 µA total vs ~17 µA per dedicated channel.
    // NEVER use attachInterrupt() for these pins: it forces GPIOTE_CONFIG_MODE_Event
    // (dedicated channel per pin, ~17 µA each).
    for (int i = 0; i < 5; i++) {
        _nav[i].pin = NAV_TABLE[i].pin;
        _nav[i].stable = false;
        _nav[i].combo = false;
        _nav[i].lastActionMs = 0;
        registerGpiotePortPin(g_ADigitalPinMap[_nav[i].pin]);
    }

    _fn.pin = PIN_BUTTON_FN;
    _fn.stable = false;
    registerGpiotePortPin(g_ADigitalPinMap[_fn.pin]);

    enableGpiotePortCallback(gpiotePortHandler);

    inputBroker->registerSource(this);

    // Start dormant; the ISR will call setIntervalFromNow(0) on the first edge.
    setInterval(INT32_MAX);

    LOG_INFO("SixKeyNavInput: PORT-event init ~2uA (Up=%d Down=%d Left=%d Right=%d Enter=%d Fn=%d)", PIN_BUTTON_UP,
             PIN_BUTTON_DOWN, PIN_BUTTON_LEFT, PIN_BUTTON_RIGHT, PIN_BUTTON_ENTER, PIN_BUTTON_FN);
    return true;
}

// ---------------------------------------------------------------------------

void SixKeyNavInput::fireEvent(input_broker_event evt)
{
    InputEvent e = {};
    e.source = _originName;
    e.inputEvent = evt;
    this->notifyObservers(&e);
}

// ---------------------------------------------------------------------------
// Fire a kbchar-routed system command (INPUT_BROKER_MSG_* defines).
// inputEvent is left as INPUT_BROKER_NONE; SystemCommandsModule routes on kbchar.
void SixKeyNavInput::fireKbEvent(uint8_t kbchar)
{
    InputEvent e = {};
    e.source = _originName;
    e.inputEvent = INPUT_BROKER_NONE;
    e.kbchar = kbchar;
    this->notifyObservers(&e);
}

// ---------------------------------------------------------------------------
// Called by the OSThread scheduler after an ISR wakes us, or periodically
// while any button is held (to service the Fn long-press shutdown timer).
//
// Design: the ISR captures a level snapshot; we consume it here and do edge
// detection by comparing against the previous thread-side state (_fn.stable /
// _nav[i].stable).  "Keep latest, process once" - if a fast press+release
// completes between two ISR edges the latest snapshot wins.
// ---------------------------------------------------------------------------
int32_t SixKeyNavInput::runOnce()
{
    // ── Consume ISR snapshot (or fall through to re-check long-press timer) ─
    // navSnap    : current level of each nav pin (for rising-edge / release detection)
    // navPressed : sticky OR since last poll (for falling-edge / press detection)
    //   Using two separate masks ensures a press that completes entirely between
    //   two thread polls (press+release during e-ink refresh) is not lost:
    //   navSnap may show 0 (released), but navPressed still has the press bit set.
    uint8_t navSnap;
    uint8_t navPressed;
    bool fnSnap;
    if (_snapshotReady) {
        // Single-core nRF52: reading volatile then clearing is safe;
        // if the ISR fires between reads it overwrites navSnapshot (fine) and
        // ORs into navPressedMask (fine - thread will pick it up next time).
        navSnap = _navSnapshot;
        navPressed = _navPressedMask;
        fnSnap = _fnSnapshot;
        _navPressedMask = 0;
        _snapshotReady = false;
    } else {
        // No new ISR snapshot - periodic poll for Fn long-press timer.
        // Re-read Fn live; synthesise nav state from thread-side stable flags
        // so the loop below sees no spurious edges.
        fnSnap = !digitalRead(_fn.pin);
        navSnap = navPressed = 0;
        for (int i = 0; i < 5; i++)
            if (_nav[i].stable)
                navSnap |= (1u << i);
        navPressed = navSnap; // held keys: navPending == navNow → no new edge
    }

    // ── Fn button ──────────────────────────────────────────────────────────
    if (fnSnap && !_fn.stable) {
        // Falling edge: Fn just pressed
        _fn.stable = true;
        _fnConsumed = false;
        _fnPressTime = millis();
    } else if (!fnSnap && _fn.stable) {
        // Rising edge: Fn just released - modifier-only, no standalone event
        _fn.stable = false;
        _fnConsumed = false;
    }

    // Long-press check: fire once when Fn held ≥ SHUTDOWN_MS
    if (_fn.stable && !_fnConsumed && (millis() - _fnPressTime) >= SHUTDOWN_MS) {
        LOG_INFO("SixKeyNav: Fn held 4s SHUTDOWN");
        if (onFnHeld)
            onFnHeld();
        _fnConsumed = true;
    }

    // ── Navigation buttons ────────────────────────────────────────────────
    // Two separate checks (not if/else) so that a press+release that completed
    // entirely between two polls can be detected in one runOnce() call:
    //   navPressed bit set  → falling edge (key was pressed since last poll)
    //   navSnap    bit clear → rising edge  (key is currently released)
    for (int i = 0; i < 5; i++) {
        bool navNow = (navSnap >> i) & 1u;        // current level
        bool navPending = (navPressed >> i) & 1u; // pressed at any point since last poll

        if (navPending && !_nav[i].stable) {
            // Falling edge: nav key was pressed since last poll
            _nav[i].stable = true;
            if (_fn.stable) {
                // ── Combo: Fn held + nav key pressed ──────────────────────
                _nav[i].combo = true;
                if (millis() - _nav[i].lastActionMs >= DEBOUNCE_MS) {
                    _nav[i].lastActionMs = millis();
                    LOG_INFO("SixKeyNav: Combo Fn+%s", NAV_TABLE[i].name);
                    if (i == 0 && onComboUp)
                        onComboUp();
                    else if (i == 1 && onComboDown)
                        onComboDown();
                    else if (i == 2 && onComboLeft)
                        onComboLeft();
                    else if (i == 3 && onComboRight)
                        onComboRight();
                    else if (i == 4 && onComboEnter)
                        onComboEnter();
                }
                _fnConsumed = true; // absorb the pending Fn release
            } else {
                // ── Normal press: record intent, fire on release ──────────
                _nav[i].combo = false;
            }
        }
        if (!navNow && _nav[i].stable) {
            // Rising edge: nav key is currently released
            _nav[i].stable = false;
            if (!_nav[i].combo) {
                // ── Normal navigation event (fired on release) ────────────
                if (millis() - _nav[i].lastActionMs >= DEBOUNCE_MS) {
                    _nav[i].lastActionMs = millis();
                    LOG_INFO("SixKeyNav: %s pressed", NAV_TABLE[i].name);
#ifdef EINK_RA01S_GP02_INKHUD
                    // InkHUD mode: call InkHUD navigation API directly.
                    // joystick.alignment=1 in nicheGraphics.h cancels rotation=3,
                    // so Up/Down/Left/Right map 1:1 to display directions.
                    //
                    // Wakeup guard: if the display panel is powered off (Fn+Down
                    // was used to cut EN), the first keypress wakes it back up
                    // instead of navigating. The keypress is consumed as wakeup.
                    if (inkHudIsDisplayOff()) {
                        inkHudWakeup();
                    } else {
                        using namespace NicheGraphics;
                        auto *inkhud = InkHUD::InkHUD::getInstance();
                        if (i == 0)
                            inkhud->navUp();
                        else if (i == 1)
                            inkhud->navDown();
                        else if (i == 2)
                            inkhud->navLeft();
                        else if (i == 3)
                            inkhud->navRight();
                        else if (i == 4)
                            inkhud->shortpress();
                    }
#else
                    // Standard UI mode: route through InputBroker → Screen.
                    fireEvent(NAV_TABLE[i].evt);
#endif
                }
            }
        }
    }

    // While any button is held: poll every 10 ms to service long-press timer
    // and catch the release edge.  Otherwise go dormant - ISR wakes us.
    bool anyPressed = _fn.stable;
    for (int i = 0; i < 5; i++)
        anyPressed |= _nav[i].stable;

#ifdef LED_EINK_ACTIVITY
    // Red LED: ON while any button is physically held, OFF when all released.
    digitalWrite(LED_EINK_ACTIVITY, anyPressed ? LED_STATE_ON : LED_STATE_OFF);
#endif

    return (anyPressed ? 10 : INT32_MAX);
}

#endif // EINK_RA01S_GP02
