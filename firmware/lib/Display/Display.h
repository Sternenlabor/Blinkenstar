#include <Arduino.h>
#include <stdint.h>

// provide dummy FW_REV_MAJOR/MINOR so static_patterns.h compiles
#ifndef FW_REV_MAJOR
#define FW_REV_MAJOR 1
#endif
#ifndef FW_REV_MINOR
#define FW_REV_MINOR 0
#endif

// --- Animation definitions ---

enum class AnimationType : uint8_t
{
    TEXT = 1,
    FRAMES = 2
};

// Struct representing an animation or pattern
struct animation
{
    AnimationType type;
    uint16_t length;
    uint8_t speed;
    uint8_t delay;
    uint8_t direction;
    uint8_t repeat;
    uint8_t *data;
};
typedef struct animation animation_t;

class Display
{
public:
    Display();
    void enable();
    void disable();
    void multiplex();
    void update();
    void reset();
    void show(animation_t *anim);

    // Diagnostics: simple column control for hardware testing
    void clearColumns();
    void setColumn(uint8_t idx, uint8_t value);

    // Activity indicator: briefly overlay a pixel without disrupting animations
    void setIndicator(uint8_t col, uint8_t row, uint8_t frames);
    void clearIndicator();

private:
    uint8_t active_col = 0;                                                 // Current column being multiplexed
    uint8_t update_cnt = 0;                                                 // Counter for animation timing
    uint8_t need_update = 0;                                                // Flag set when a new frame/scroll is needed
    uint8_t update_threshold = 0;                                           // How many column-cycles per animation step
    uint8_t disp_buf[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Column data buffer
    uint8_t str_pos = 0;                                                    // Position index within animation data
    uint8_t str_chunk = 0;                                                  // For multi-chunk animations (unused here)
    int8_t char_pos = -1;                                                   // For text animations (start at -1)
    uint8_t repeat_cnt = 0;                                                 // Repeat counter (unused in this port)
    enum AnimationStatus : uint8_t
    {
        RUNNING,
        SCROLL_BACK,
        PAUSED
    };
    AnimationStatus status = RUNNING; // Current animation status
    animation_t *current_anim = nullptr;

    // Indicator overlay state
    bool indicator_active = false;
    uint8_t indicator_col = 0;
    uint8_t indicator_row = 0;
    uint8_t indicator_frames = 0; // decremented once per full refresh
};

extern Display display;
