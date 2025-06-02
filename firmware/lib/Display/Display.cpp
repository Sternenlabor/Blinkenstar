#include "Display.h"
#include "Timer.h"
#include <avr/pgmspace.h>
#include "font.h"
#include "static_patterns.h" // Contains turnonPattern[] or text patterns in PROGMEM

static Timer timer;
Display display; // Global display instance

// Buffers for the static turn-on pattern
static animation_t activeAnimation;
static uint8_t activeAnimationData[128];

// Timer interrupt handler: calls multiplexing
static void onTimerTick()
{
    display.multiplex();
}

Display::Display()
{
    // Constructor: initial values are already set by in-class initializers
}

// Enable display hardware and timer interrupt
void Display::enable()
{
    // Configure ports
    DDRB = 0xFF; // Columns output
    DDRD = 0xFF; // Rows output

    // Start Timer1 at 60 Hz to drive multiplexing
    timer.initialize(60); //60 µs period
    timer.attachInterrupt(onTimerTick);
    timer.start();

    // --- Initialize turn-on animation pattern ---
    const uint8_t *pattern = emptyPattern; // PROGMEM pointer
    uint8_t hdr0 = pgm_read_byte(pattern);
    uint8_t hdr1 = pgm_read_byte(pattern + 1);

    activeAnimation.type = (AnimationType)(hdr0 >> 4);
    activeAnimation.length = ((hdr0 & 0x0F) << 8) | hdr1;

    uint8_t p2 = pgm_read_byte(pattern + 2);
    uint8_t p3 = pgm_read_byte(pattern + 3);

    if (activeAnimation.type == AnimationType::TEXT)
    {
        activeAnimation.speed = 250 - (p2 & 0xf0);
        activeAnimation.delay = (p2 & 0x0f);
        activeAnimation.direction = (p3 >> 4);
        activeAnimation.repeat = (p3 & 0x0f);
    }
    else if (activeAnimation.type == AnimationType::FRAMES)
    {
        activeAnimation.speed = 250 - ((p2 & 0x0F) << 4);
        activeAnimation.delay = (p3 >> 4);
        activeAnimation.direction = 0;
        activeAnimation.repeat = (p3 & 0x0F);
    }

    // Copy the column data bytes into RAM
    uint16_t len = activeAnimation.length;
    if (len > 128)
    {
        len = 128;
    }
    for (uint16_t i = 0; i < len; i++)
    {
        activeAnimationData[i] = pgm_read_byte(pattern + 4 + i);
    }
    activeAnimation.data = activeAnimationData;

    // Show the turn-on animation and render the first frame
    show(&activeAnimation);
    need_update = 1;
    update();
}

// Multiplex one column; advance animation on threshold
void Display::multiplex()
{
    // Disable current column
    PORTB = 0x00;
    // Output row data for the active column
    PORTD = disp_buf[active_col];
    // Enable the active column (active-low)
    PORTB = (1 << active_col);

    // Next column
    if (++active_col == 8)
    {
        active_col = 0;
        if (++update_cnt == update_threshold)
        {
            update_cnt = 0;
            need_update = 1;
            update();
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

// Start a new animation
void Display::show(animation_t *anim)
{
    current_anim = anim;
    reset();
    update_threshold = current_anim->speed;
    if (current_anim->direction == 1)
    {
        // If reverse direction, start at end for TEXT or FRAMES
        if (current_anim->length > 0)
        {
            str_pos = current_anim->length - 1;
        }
    }
}

// Update display content when needed (called from interrupt)
void Display::update()
{
    if (!need_update || !current_anim)
    {
        return;
    }
    need_update = 0;

    if (status == RUNNING)
    {
        if (current_anim->type == AnimationType::FRAMES)
        {
            // Copy one frame (8 columns) into disp_buf
            for (uint8_t i = 0; i < 8; i++)
            {
                disp_buf[i] = ~(current_anim->data[str_pos + i]); // invert for active-low
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
            const uint8_t *glyph_addr = (const uint8_t *)pgm_read_ptr(&font[current_anim->data[str_pos]]);
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

            // Append one column (active-low)
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
