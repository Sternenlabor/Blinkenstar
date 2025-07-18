#include "System.h"
#include "Display.h"
#include "static_patterns.h"
#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

System rocket;

static uint8_t animationBuffer[128];

// Arduino pin definitions for buttons (PC3 -> A3, PC7 -> A7)
constexpr uint8_t BUTTON1_PIN = A3;
constexpr uint8_t BUTTON2_PIN = A7;

void System::initialize()
{
    // Disable watchdog via Arduino API
    wdt_disable();

    // Configure button pins with internal pull-ups
    pinMode(BUTTON1_PIN, INPUT_PULLUP);
    pinMode(BUTTON2_PIN, INPUT_PULLUP);

    // Initialize display
    display.enable();

    // Enable interrupts globally
    interrupts();
}

void System::loop()
{
    // Check if both buttons are pressed (active-low) for a shutdown request (long press on both buttons)
    if (digitalRead(BUTTON1_PIN) == LOW && digitalRead(BUTTON2_PIN) == LOW)
    {
        /*
         * Naptime!
         * (But not before both buttons have been pressed for at least
         * SHUTDOWN_THRESHOLD * 0.256 ms)
         */
        if (want_shutdown < SHUTDOWN_THRESHOLD)
        {
            want_shutdown++;
        }
        else
        {
            shutdown();
            want_shutdown = 0;
        }
    }
    else
    {
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
    while (digitalRead(BUTTON1_PIN) == LOW || digitalRead(BUTTON2_PIN) == LOW)
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
    PCMSK1 |= _BV(PCINT11) | _BV(PCINT15);
    PCICR |= _BV(PCIE1);

    // Enter deep sleep (power-down)
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    interrupts();
    sleep_cpu();
    sleep_disable();

    // After wakeup, disable pin-change interrupts
    PCMSK1 &= ~(_BV(PCINT11) | _BV(PCINT15));

    // Re-enable display
    display.enable();

    // Wait for buttons release post-wakeup
    while (digitalRead(BUTTON1_PIN) == LOW || digitalRead(BUTTON2_PIN) == LOW)
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
