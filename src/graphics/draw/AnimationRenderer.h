#pragma once
#if defined(EINK_RA01S_GP02)
#if HAS_SCREEN
#include <OLEDDisplay.h>
#include <OLEDDisplayUi.h>

namespace graphics
{
namespace AnimationRenderer
{

/// Called from Screen::setFrames() — true means it's time for the next frame
/// and the screen should trigger a fast redraw.
bool needsRedraw();

/// FrameCallback registered in setFrames() as the first normal frame.
void drawAnimationFrame(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);

/// Switch active animation (idx 0-based into ANIM_LIST), resets frame counter.
void setAnimation(uint8_t idx);

/// Total number of available animations (= ANIM_COUNT).
uint8_t getAnimCount();

/// Display name for animation at idx.
const char *getAnimName(uint8_t idx);

} // namespace AnimationRenderer
} // namespace graphics

#endif // HAS_SCREEN
#endif // EINK_RA01S_GP02