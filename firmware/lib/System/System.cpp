#include "System.h"
#include "Display.h"
#include "static_patterns.h"
#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/io.h>
#ifdef ENABLE_MODEM
#include "Receiver.h"
#endif

System rocket;

static uint8_t animationBuffer[128];

// Buttons are wired to Port C pins: PC3 (pin 26) and PC7 (pin 20)
// Use direct port access to avoid any Arduino pin mapping ambiguity
static inline bool button1_is_low() { return (PINC & _BV(PC3)) == 0; }
static inline bool button2_is_low() { return (PINC & _BV(PC7)) == 0; }

void System::initialize()
{
    // Disable watchdog via Arduino API
    wdt_disable();

    // Configure button pins with internal pull-ups on Port C (PC3, PC7)
    DDRC &= ~( _BV(PC3) | _BV(PC7) );   // inputs
    PORTC |= _BV(PC3) | _BV(PC7);       // enable pull-ups

    // Initialize display
    display.enable();

    // Start audio receiver (stores incoming data via Storage)
#ifdef ENABLE_MODEM
    modemReceiver.begin();
#endif

    // Enable interrupts globally
    interrupts();
}

void System::loop()
{
    // Process any incoming audio data
#ifdef ENABLE_MODEM
    modemReceiver.process();
#endif

    // Check if both buttons are pressed (active-low) for a shutdown request
    bool both_low = button1_is_low() && button2_is_low();

    if (both_low)
    {
        // Require a short stable period with both buttons low
        if (both_pressed_stable < BOTH_STABLE_THRESHOLD)
        {
            both_pressed_stable++;
            return; // don't start counting toward shutdown yet
        }

        /*
         * Naptime!
         * (After BOTH_STABLE_THRESHOLD stable polls, count at
         * SHUTDOWN_THRESHOLD * poll-period)
         */
        if (want_shutdown < SHUTDOWN_THRESHOLD)
        {
            want_shutdown++;
        }
        else
        {
            shutdown();
            want_shutdown = 0;
            both_pressed_stable = 0;
        }
    }
    else
    {
        // Any release or mismatch resets the detection
        both_pressed_stable = 0;
        want_shutdown = 0;
    }
}

void System::shutdown()
{
    uint8_t i;

    // Load shutdown animation from PROGMEM
    const uint8_t *pattern = shutdownPattern;
    animation_t anim;

    // Read headers
    uint8_t hdr0 = pgm_read_byte(pattern);
    uint8_t hdr1 = pgm_read_byte(pattern + 1);
    anim.type = static_cast<AnimationType>(hdr0 >> 4);
    anim.length = ((hdr0 & 0x0F) << 8) | hdr1;

    // Parse parameters for frame-based animations
    uint8_t p2 = pgm_read_byte(pattern + 2);
    uint8_t p3 = pgm_read_byte(pattern + 3);
    if (anim.type == AnimationType::FRAMES)
    {
        anim.speed = 250 - ((p2 & 0x0F) << 4);
        anim.delay = p3 >> 4;
    }

    // Copy data to RAM buffer using Arduino min() macro
    uint16_t len = min(anim.length, (uint16_t)sizeof(animationBuffer));
    for (uint16_t i = 0; i < len; ++i)
    {
        animationBuffer[i] = pgm_read_byte(pattern + 4 + i);
    }
    anim.data = animationBuffer;

    // Display shutdown animation
    display.show(&anim);

    // Wait until buttons are released
    while (button1_is_low() || button2_is_low())
    {
        display.update();
    }

    // and some more to debounce (~100ms) the buttons (and finish powerdown animation)
    for (i = 0; i < 100; i++)
    {
        display.update();
        delay(1);
    }

    // Turn off display
    display.disable();

    // Disable ADC to save power via Arduino API
    power_adc_disable();

    // Enable pin-change interrupts on button pins for wakeup
    // Map: PC3 (A3) -> PCINT11 -> PCMSK1 bit 3, PC7 (A7) -> PCINT15 -> PCMSK1 bit 7
    PCMSK1 |= _BV(3) | _BV(7);
    PCICR |= _BV(PCIE1);

    // Enter deep sleep (power-down)
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    interrupts();
    sleep_cpu();
    sleep_disable();

    // After wakeup, disable pin-change interrupts
    PCMSK1 &= ~( _BV(3) | _BV(7) );

    // Re-enable display
    display.enable();

    // Wait for buttons release post-wakeup
    while (button1_is_low() || button2_is_low())
    {
        display.update();
    }

    // Debounce again
    for (i = 0; i < 100; i++)
    {
        display.update();
        delay(1);
    }

    // Re-enable ADC
    power_adc_enable();
}

// ISR for wakeup via button presses
ISR(PCINT1_vect)
{
    // No action needed; wake-up is automatic
}
