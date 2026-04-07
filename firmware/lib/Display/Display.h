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

struct DisplayState
{
    uint8_t columns[8];
    bool indicator_active;
    uint8_t indicator_col;
    uint8_t indicator_row;
    uint8_t indicator_frames;
    bool boot_message_active;
    bool animation_active;
    bool animation_storage_backed;
    animation_t animation;
};

class Display
{
public:
    /**
     * Enable matrix GPIOs and start the multiplex timer.
     */
    void enable();

    /**
     * Disable matrix output and stop the multiplex timer.
     */
    void disable();

    /**
     * Drive one multiplex column and advance the animation counters.
     */
    void multiplex();

    /**
     * Advance the currently active animation when the refresh threshold is reached.
     */
    void update();

    /**
     * Reset animation counters and blank the display buffer.
     */
    void reset();

    /**
     * Start showing an animation that already lives in RAM.
     *
     * @param anim Animation descriptor to copy and display.
     */
    void show(const animation_t *anim);

    /**
     * Start showing an animation whose payload can be reloaded from EEPROM in 128-byte chunks.
     *
     * @param anim Animation descriptor to copy and display.
     */
    void showFromStorage(const animation_t *anim);

    /**
     * Start the built-in boot message streamed from PROGMEM.
     */
    void showBootMessage();

    /**
     * Clear every column in the display buffer.
     */
    void clearColumns();

    /**
     * Overwrite one display column in the backing buffer.
     *
     * @param idx Column index.
     * @param value Active-low row mask for the column.
     */
    void setColumn(uint8_t idx, uint8_t value);

    /**
     * Overlay a transient activity pixel without disturbing the main animation state.
     *
     * @param col Indicator column.
     * @param row Indicator row.
     * @param frames Number of full refreshes to keep the indicator visible.
     */
    void setIndicator(uint8_t col, uint8_t row, uint8_t frames);

    /**
     * Remove any transient activity indicator pixel.
     */
    void clearIndicator();

    /**
     * Return and clear a pending autoskip request raised by finite-repeat playback.
     *
     * @returns `true` once after an animation reaches its repeat limit.
     */
    bool consumeAnimationRepeatRequest();

    /**
     * Capture the visible display state for later restoration.
     *
     * @param state Destination snapshot.
     */
    void snapshotState(DisplayState &state) const;

    /**
     * Restore a snapshot as a frozen static frame.
     *
     * @param state Snapshot to freeze into the display buffer.
     */
    void freezeState(const DisplayState &state);

    /**
     * Restore a snapshot, restarting animations or the boot message when needed.
     *
     * @param state Snapshot to restore.
     */
    void restoreState(const DisplayState &state);

private:
    /**
     * Copy an animation descriptor into the active slot and initialize playback state.
     *
     * @param anim Animation descriptor to copy.
     * @param storage_backed `true` when later payload chunks can be reloaded from EEPROM.
     */
    void startAnimation_(const animation_t *anim, bool storage_backed);

    /**
     * Ensure that the chunk containing the current payload byte is present in RAM.
     */
    void ensureStorageChunkLoaded_();

    /**
     * Return the byte offset of `str_pos` inside the currently loaded chunk.
     *
     * @returns Chunk-local byte offset.
     */
    uint8_t chunkOffset_() const;

    /**
     * Rewind the current animation to its next-cycle start position.
     */
    void rewindAnimationCycle_();

    /**
     * Apply the upstream end-of-animation pause and repeat handling.
     *
     * @returns `true` when the current cycle autoskipped to another pattern.
     */
    bool finishAnimationCycle_();

    uint8_t active_col;       // Current column being multiplexed
    uint8_t update_cnt;       // Counter for animation timing
    uint8_t need_update;      // Flag set when a new frame/scroll is needed
    uint8_t update_threshold; // How many column-cycles per animation step
    uint8_t disp_buf[8];      // Column data buffer
    uint16_t str_pos;         // Position index within animation data
    uint8_t str_chunk;        // Active 128-byte EEPROM chunk for storage-backed playback
    int8_t char_pos;          // For text animations (start at -1)
    uint8_t repeat_cnt;       // Upstream-compatible finite-repeat counter
    enum AnimationStatus : uint8_t
    {
        RUNNING,
        SCROLL_BACK,
        PAUSED
    };
    AnimationStatus status; // Current animation status
    animation_t *current_anim;
    animation_t current_anim_copy;
    bool current_anim_progmem;
    bool current_anim_storage_backed;
    bool repeat_advance_requested_;

    // Indicator overlay state
    bool indicator_active;
    uint8_t indicator_col;
    uint8_t indicator_row;
    uint8_t indicator_frames; // decremented once per full refresh
};

extern Display display;
