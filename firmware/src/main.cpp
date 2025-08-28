#include <Arduino.h>
#include <avr/io.h>
#include "System.h"
#include "Display.h"

#ifdef DIAG_BUTTONS

// Buttons on Port C: PC3 (pin 26), PC7 (pin 20)
static inline bool btn_pc3_low() { return (PINC & _BV(PC3)) == 0; }
static inline bool btn_pc7_low() { return (PINC & _BV(PC7)) == 0; }

void setup()
{
    // Configure buttons as inputs with pull-ups
    DDRC &= ~( _BV(PC3) | _BV(PC7) );
    PORTC |= _BV(PC3) | _BV(PC7);

    // Start display multiplexing timer
    display.enable();
}

void loop()
{
    // Update columns based on button state
    bool b1 = btn_pc3_low();
    bool b2 = btn_pc7_low();

    display.clearColumns();
    if (b1)
    {
        // Light column 0 fully when PC3 low
        display.setColumn(0, 0x00);
    }
    if (b2)
    {
        // Light column 7 fully when PC7 low
        display.setColumn(7, 0x00);
    }

    // Light center pixel if both pressed (visual XOR -> both gives center)
    if (b1 && b2)
    {
        // Center row bit is bit 3 (0-indexed from LSB); active-low rows
        display.setColumn(3, (uint8_t)~_BV(3));
        display.setColumn(4, (uint8_t)~_BV(4));
    }

    // Small delay to slow down polling; multiplex runs independently
    delay(2);
}

#else

void setup()
{
    // Initialize system (sets up display and timer)
    rocket.initialize();
}

void loop()
{
    // Continuously run system loop (not used in this case, as everything is interrupt-driven)
    rocket.loop();
}

#endif
