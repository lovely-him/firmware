#if defined(EINK_RA01S_GP02)
#include "configuration.h"
#if HAS_SCREEN
#include "AnimationRenderer.h"
#include "graphics/ScreenFonts.h"
#include "graphics/SharedUIDisplay.h"

// Board-specific frame data is provided by anim_frames.h, placed in the
// variant include directory and gated by EINK_RA01S_GP02.
// The header defines:
//   ANIM_FRAME_WIDTH / ANIM_FRAME_HEIGHT / ANIM_BYTES_PER_FRAME
//   ANIM_COUNT                      — total number of animations
//   AnimDesc { name, data, frameCount }
//   ANIM_LIST[ANIM_COUNT]           — descriptor table
#include "anim_frames.h"

namespace graphics
{
namespace AnimationRenderer
{

namespace
{
static uint8_t s_animIdx = 0;
static uint8_t s_frameIdx = 0;
static uint32_t s_lastAdvance = 0;
static constexpr uint32_t FRAME_INTERVAL_MS = 3000;
} // namespace

bool needsRedraw()
{
    return (millis() - s_lastAdvance) >= FRAME_INTERVAL_MS;
}

void drawAnimationFrame(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
#if 1
    // Advance to the next frame when the interval has elapsed.
    if (millis() - s_lastAdvance >= FRAME_INTERVAL_MS) {
        s_frameIdx = (s_frameIdx + 1) % ANIM_LIST[s_animIdx].frameCount;
        s_lastAdvance = millis();
    }

    // Compute frame pointer: flat array, each frame is ANIM_BYTES_PER_FRAME bytes.
    const uint8_t *frame = ANIM_LIST[s_animIdx].data + (size_t)s_frameIdx * ANIM_BYTES_PER_FRAME;

    // Center the image on screen.
    const int16_t imgX = x + (display->getWidth() - ANIM_FRAME_WIDTH) / 2;
    const int16_t imgY = y + (display->getHeight() - ANIM_FRAME_HEIGHT) / 2;

    display->drawXbm(imgX, imgY, ANIM_FRAME_WIDTH, ANIM_FRAME_HEIGHT, frame);
#else
    // Fallback: nothing to draw on boards without anim_frames.h
    (void)display;
    (void)state;
    (void)x;
    (void)y;
#endif
}

void setAnimation(uint8_t idx)
{
    if (idx < ANIM_COUNT) {
        s_animIdx = idx;
        s_frameIdx = 0;
        s_lastAdvance = millis();
    }
}

uint8_t getAnimCount()
{
    return ANIM_COUNT;
}

const char *getAnimName(uint8_t idx)
{
    return idx < ANIM_COUNT ? ANIM_LIST[idx].name : "";
}

} // namespace AnimationRenderer
} // namespace graphics

#endif // HAS_SCREEN
#endif // EINK_RA01S_GP02
