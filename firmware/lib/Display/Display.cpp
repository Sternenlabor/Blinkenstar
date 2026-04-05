#include "Display.h"
#include "Storage.h"
#include "Timer.h"
#include <avr/pgmspace.h>
#include "font.h"
#include "static_patterns.h" // Contains turnonPattern[] or text patterns in PROGMEM

static Timer timer;
Display display; // Global display instance
static constexpr uint8_t kBootMessageLength = sizeof(emptyPattern) - 4;

/**
 * Forward timer interrupts into the display multiplex routine.
 */
static void onTimerTick()
{
    display.multiplex();
}

Display::Display()
{
    // Constructor: initial values are already set by in-class initializers
}

void Display::startAnimation_(const animation_t *anim, bool storage_backed)
{
    current_anim_copy = *anim;
    current_anim = &current_anim_copy;
    current_anim_progmem = false;
    current_anim_storage_backed = storage_backed;
    reset();
    update_threshold = current_anim->speed;

    if (current_anim->direction == 1 && current_anim->length > 0)
    {
        str_pos = current_anim->length - 1;
    }

    // Render the first frame immediately so the caller does not need to wait
    // for the next timer threshold before seeing the new content.
    need_update = 1;
    update();
}

void Display::ensureStorageChunkLoaded_()
{
    if (current_anim_storage_backed && current_anim->length > 128)
    {
        uint8_t desired_chunk = str_pos / 128;

        if (desired_chunk != str_chunk)
        {
            str_chunk = desired_chunk;
            // Storage::load() already selected the active pattern and latched
            // its page offset, so later chunk loads can stream the rest of the
            // payload into the same 128-byte RAM window.
            storage.loadChunk(str_chunk, current_anim->data);
        }
    }
}

uint8_t Display::chunkOffset_() const
{
    if (current_anim_storage_backed && current_anim->length > 128)
    {
        return str_pos % 128;
    }

    return str_pos;
}

// Enable display hardware and timer interrupt
void Display::enable()
{
    // Configure ports
    DDRB = 0xFF; // Columns output
    DDRD = 0xFF; // Rows output

    // Match the upstream multiplex cadence: one column every 256 us,
    // which yields a full 8-column refresh every 2048 us.
    timer.initialize(256);
    timer.attachInterrupt(onTimerTick);
    timer.start();

    // --- Initialize turn-on animation pattern (skip in diagnostics or when disabled) ---
#if (!defined(DIAG_BUTTONS) || defined(DIAG_BOOT_MESSAGE)) && !defined(NO_BOOT_MESSAGE)
    showBootMessage();
#endif
}

// Multiplex one column; advance animation on threshold
void Display::multiplex()
{
    // Disable current column
    PORTB = 0x00;
    // Output row data for the active column
    uint8_t rows = disp_buf[active_col];
    // Overlay indicator pixel (active-low)
    if (indicator_active && active_col == indicator_col)
    {
        rows &= (uint8_t)~_BV(indicator_row);
    }
    PORTD = rows;
    // The matrix uses active-low column selection, so driving one bit high selects that transistor.
    PORTB = (1 << active_col);

    // Next column
    if (++active_col == 8)
    {
        active_col = 0;
        // Decrement indicator lifetime once per full refresh
        if (indicator_active)
        {
            if (indicator_frames > 0)
            {
                indicator_frames--;
            }
            if (indicator_frames == 0)
            {
                indicator_active = false;
            }
        }
        if (++update_cnt == update_threshold)
        {
            update_cnt = 0;
            need_update = 1;
        }
    }
}

void Display::disable()
{
    timer.stop();
    DDRB = DDRD = 0x00;
}

// Reset the display buffer and animation state
void Display::reset()
{
    for (uint8_t i = 0; i < 8; i++)
    {
        disp_buf[i] = 0xFF; // All LEDs off (active-low)
    }
    update_cnt = 0;
    repeat_cnt = 0;
    str_pos = 0;
    str_chunk = 0;
    char_pos = -1;
    need_update = 0;
    status = RUNNING;
}

// --- Diagnostics helpers ---
void Display::clearColumns()
{
    for (uint8_t i = 0; i < 8; i++)
    {
        // 0xFF = all LEDs off for a column (active-low rows)
        // Keep private buffer semantics consistent
        // (Multiplex ISR will read from disp_buf)
        // Note: no locking; acceptable for simple diagnostics
        disp_buf[i] = 0xFF;
    }
}

void Display::setColumn(uint8_t idx, uint8_t value)
{
    if (idx < 8)
    {
        disp_buf[idx] = value;
    }
}

void Display::setIndicator(uint8_t col, uint8_t row, uint8_t frames)
{
    if (col < 8 && row < 8)
    {
        indicator_col = col;
        indicator_row = row;
        indicator_frames = frames;
        indicator_active = frames > 0;
    }
}

void Display::clearIndicator()
{
    indicator_active = false;
    indicator_frames = 0;
}

void Display::snapshotState(DisplayState &state) const
{
    for (uint8_t i = 0; i < 8; ++i)
    {
        state.columns[i] = disp_buf[i];
    }
    state.indicator_active = indicator_active;
    state.indicator_col = indicator_col;
    state.indicator_row = indicator_row;
    state.indicator_frames = indicator_frames;
    state.boot_message_active = current_anim == nullptr && current_anim_progmem;
    state.animation_active = current_anim != nullptr;
    state.animation_storage_backed = false;
    if (state.animation_active)
    {
        state.animation = current_anim_copy;
        state.animation_storage_backed = current_anim_storage_backed;
    }
}

void Display::freezeState(const DisplayState &state)
{
    for (uint8_t i = 0; i < 8; ++i)
    {
        disp_buf[i] = state.columns[i];
    }

    indicator_active = state.indicator_active;
    indicator_col = state.indicator_col;
    indicator_row = state.indicator_row;
    indicator_frames = state.indicator_frames;

    // Restore the saved visible frame as a static image. This prevents the
    // temporary shutdown animation from continuing to run after wake.
    current_anim = nullptr;
    current_anim_progmem = false;
    current_anim_storage_backed = false;
    update_cnt = 0;
    need_update = 0;
    update_threshold = 0;
    str_pos = 0;
    str_chunk = 0;
    char_pos = -1;
    repeat_cnt = 0;
    status = RUNNING;
}

void Display::restoreState(const DisplayState &state)
{
    if (state.boot_message_active)
    {
        showBootMessage();
        indicator_active = state.indicator_active;
        indicator_col = state.indicator_col;
        indicator_row = state.indicator_row;
        indicator_frames = state.indicator_frames;
        return;
    }

    if (state.animation_active)
    {
        if (state.animation_storage_backed)
        {
            showFromStorage(&state.animation);
        }
        else
        {
            show(&state.animation);
        }
        indicator_active = state.indicator_active;
        indicator_col = state.indicator_col;
        indicator_row = state.indicator_row;
        indicator_frames = state.indicator_frames;
        return;
    }

    freezeState(state);
}

// Start a new animation
void Display::show(const animation_t *anim)
{
    startAnimation_(anim, false);
}

void Display::showFromStorage(const animation_t *anim)
{
    startAnimation_(anim, true);
}

void Display::showBootMessage()
{
    current_anim = nullptr;
    current_anim_progmem = true;
    current_anim_storage_backed = false;
    reset();

    const uint8_t p2 = pgm_read_byte(emptyPattern + 2);
    const uint8_t p3 = pgm_read_byte(emptyPattern + 3);
    const uint8_t hdr0 = pgm_read_byte(emptyPattern);
    const AnimationType type = static_cast<AnimationType>(hdr0 >> 4);

    if (type == AnimationType::TEXT)
    {
        update_threshold = 250 - (p2 & 0xF0);
    }
    else
    {
        update_threshold = 250 - ((p2 & 0x0F) << 4);
    }

    if ((p3 >> 4) == 1)
    {
        const uint8_t hdr1 = pgm_read_byte(emptyPattern + 1);
        const uint16_t length = ((hdr0 & 0x0F) << 8) | hdr1;
        if (length > 0)
        {
            str_pos = length - 1;
        }
    }

    need_update = 1;
    update();
}

// Update display content when needed (called from interrupt)
void Display::update()
{
    if (!need_update)
    {
        return;
    }
    need_update = 0;

    const bool boot_message_active = current_anim == nullptr && current_anim_progmem;

    if (!current_anim && !boot_message_active)
    {
        return;
    }

    if (boot_message_active)
    {
        if (status == RUNNING)
        {
            for (uint8_t i = 0; i < 7; i++)
            {
                disp_buf[i] = disp_buf[i + 1];
            }

            uint8_t glyphIndex = pgm_read_byte(emptyPattern + 4 + str_pos);
            const uint8_t *glyph_addr = (const uint8_t *)pgm_read_ptr(&font[glyphIndex]);
            uint8_t glyph_len = pgm_read_byte(&glyph_addr[0]);
            char_pos++;

            if (char_pos > glyph_len)
            {
                char_pos = 0;
                str_pos = (str_pos + 1) % kBootMessageLength;
            }

            if (char_pos == 0)
            {
                disp_buf[7] = 0xFF;
            }
            else
            {
                disp_buf[7] = ~pgm_read_byte(&glyph_addr[char_pos]);
            }
        }
        else if (status == PAUSED)
        {
            str_pos = 0;
            status = RUNNING;
        }
        return;
    }

    if (status == RUNNING)
    {
        if (current_anim->type == AnimationType::FRAMES)
        {
            ensureStorageChunkLoaded_();
            uint8_t chunk_offset = chunkOffset_();

            // Copy one frame (8 columns) into disp_buf
            for (uint8_t i = 0; i < 8; i++)
            {
                disp_buf[i] = ~(current_anim->data[chunk_offset + i]); // invert for active-low
            }
            str_pos += 8;
            // Loop back when reaching end of animation
            if (str_pos >= current_anim->length)
            {
                str_pos = 0;
            }
        }
        else if (current_anim->type == AnimationType::TEXT)
        {
            ensureStorageChunkLoaded_();
            uint8_t chunk_offset = chunkOffset_();

            // Scroll display contents
            if (current_anim->direction == 0)
            {
                // Left
                for (uint8_t i = 0; i < 7; i++)
                {
                    disp_buf[i] = disp_buf[i + 1];
                }
            }
            else
            {
                // Right
                for (uint8_t i = 7; i > 0; i--)
                {
                    disp_buf[i] = disp_buf[i - 1];
                }
            }

            // Load current character glyph from PROGMEM
            const uint8_t *glyph_addr = (const uint8_t *)pgm_read_ptr(&font[current_anim->data[chunk_offset]]);
            uint8_t glyph_len = pgm_read_byte(&glyph_addr[0]);
            char_pos++;

            if (char_pos > glyph_len)
            {
                char_pos = 0;
                // Advance to next/previous character in text
                if (current_anim->direction == 0)
                {
                    str_pos = (str_pos + 1) % current_anim->length;
                }
                else
                {
                    str_pos = (str_pos == 0) ? (current_anim->length - 1) : (str_pos - 1);
                }
            }

            // Glyphs are streamed one column at a time so long text can scroll without a full frame buffer.
            if (current_anim->direction == 0)
            {
                // New data on rightmost column
                if (char_pos == 0)
                    disp_buf[7] = 0xFF; // whitespace
                else
                    disp_buf[7] = ~pgm_read_byte(&glyph_addr[char_pos]);
            }
            else
            {
                // New data on leftmost column
                if (char_pos == 0)
                    disp_buf[0] = 0xFF; // whitespace
                else
                    disp_buf[0] = ~pgm_read_byte(&glyph_addr[glyph_len - char_pos + 1]);
            }
        }
    }
    else if (status == PAUSED)
    {
        str_pos++;
        if (str_pos >= current_anim->delay)
        {
            if (current_anim->direction == 0)
            {
                str_pos = 0;
            }
            else if (current_anim->length <= 128)
            {
                str_pos = current_anim->length - 1;
            }
            else
            {
                str_pos = (current_anim->length - 1) % 128;
            }
            status = RUNNING;
            update_threshold = current_anim->speed;
        }
    }
}
