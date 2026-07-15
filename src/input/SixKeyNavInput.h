#pragma once

#if defined(EINK_RA01S_GP02)

#include "InputBroker.h"
#include "concurrency/OSThread.h"

/**
 * SixKeyNavInput — 6-button navigation driver for eink-ra01s-gp02.
 *
 * Buttons (all ACTIVE_LOW with hardware pull-up):
 *   Up, Down, Left, Right, Enter, Fn
 *
 * Normal events (Fn NOT held):
 *   Up    → INPUT_BROKER_UP
 *   Down  → INPUT_BROKER_DOWN
 *   Left  → INPUT_BROKER_LEFT
 *   Right → INPUT_BROKER_RIGHT
 *   Enter → INPUT_BROKER_SELECT
 *   Fn short press → (no-op: Fn is modifier-only, not a standalone event)
 *   Fn ≥ 4 s       → INPUT_BROKER_SHUTDOWN  (fires once, release suppressed)
 *
 * Combo events (Fn held while pressing another key):
 *   Fn + Up/Down/Left/Right/Enter → onCombo* callbacks (assigned externally).
 *   The pending Fn release is absorbed when a combo fires.
 *
 * Interrupt-driven: all 6 pins use nrfx_gpiote in LOW-ACCURACY mode, which
 * routes through the shared GPIOTE PORT event (GPIO SENSE/DETECT) — zero
 * dedicated GPIOTE channels consumed, ~2.4 µA total System ON sleep current.
 * DO NOT use attachInterrupt(): it forces GPIOTE_CONFIG_MODE_Event (dedicated
 * channel per pin, ~17 µA each).  The file-local gpiotePortHandler wakes the
 * thread; runOnce() reads pin states and dispatches events.  While Fn is held
 * the thread self-schedules every 100 ms for the long-press shutdown timer.
 * Hardware debounce is assumed; no software debounce state machine is used.
 */
class SixKeyNavInput : public Observable<const InputEvent *>, public concurrency::OSThread
{
  public:
    const char *_originName;

    SixKeyNavInput();
    bool init();
    int32_t runOnce() override;

    // Combo-key callbacks: assign externally to inject behavior.
    // Left as nullptr → combo is silently ignored.
    void (*onComboUp)();
    void (*onComboDown)();
    void (*onComboLeft)();
    void (*onComboRight)();
    void (*onComboEnter)();
    void (*onFnHeld)();

    void fireEvent(input_broker_event evt);
    void fireKbEvent(uint8_t kbchar); // kbchar-routed system commands

    // Called from the file-local ISR (gpiotePortHandler) to sample all pin
    // levels immediately and store them as a volatile snapshot for runOnce().
    void captureSnapshotFromISR();

  private:
    static const uint32_t SHUTDOWN_MS = 4000; // Fn hold threshold for shutdown
    static const uint32_t DEBOUNCE_MS = 20;   // Minimum ms between repeated key action dispatches

    struct BtnState {
        uint8_t pin;
        bool stable;           // thread-side "previous pressed" state for edge detection
        bool combo;            // true = this press was a Fn+nav combo (suppress release event)
        uint32_t lastActionMs; // debounce: millis() when an action was last dispatched for this key
    };

    // Navigation buttons (indices 0-4: Up, Down, Left, Right, Enter)
    BtnState _nav[5];

    // Fn button — modifier key: triggers combos (hold+nav) or shutdown (hold≥4s)
    BtnState _fn;
    bool _fnConsumed;      // true: this press already produced a combo/shutdown
    uint32_t _fnPressTime; // millis() when Fn first became pressed

    // ISR → thread snapshot (written in ISR, read in thread).
    // navSnapshot bit i = 1 means nav[i] is pressed (active-low: pin LOW).
    // navPressedMask is a sticky OR: bits are set when a press is sampled by the
    //   ISR and cleared by the thread after consumption.  This preserves press
    //   events even when press+release completes while the thread is blocked
    //   (e.g. during an e-ink refresh) and would otherwise overwrite navSnapshot.
    // fnSnapshot = true means Fn is pressed.
    // snapshotReady is set by ISR; thread clears it after consuming.
    volatile uint8_t _navSnapshot;
    volatile uint8_t _navPressedMask;
    volatile bool _fnSnapshot;
    volatile bool _snapshotReady;

    // Shared ISR for all 6 pins (GPIOTE PORT event via INPUT_PULLUP_SENSE)
    static void isrHandler();
};

extern SixKeyNavInput *sixKeyNavInput;

#endif // EINK_RA01S_GP02
