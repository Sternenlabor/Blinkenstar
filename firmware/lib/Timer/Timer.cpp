#include "Timer.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// Define static members
void (*Timer::callback)() = nullptr;
uint8_t Timer::clockSelectBits = 0;

void Timer::initialize(unsigned int period_us)
{
    // Stop timer and disable Timer1 interrupts during configuration
    stop();

    // Configure Timer1 for CTC mode (Clear Timer on Compare Match) using OCR1A as TOP
    TCCR1A = 0;             // Normal operation (no PWM)
    TCCR1B = 0;             // Stop timer and clear control bits
    TCCR1B |= (1 << WGM12); // WGM12 = 1: CTC mode, OCR1A as top

    // Compute OCR1A value and prescaler for the given period (in microseconds)
    if (period_us == 0)
    {
        // Special case: maximum frequency (interrupt on every tick)
        clockSelectBits = 0x01; // No prescaler, CS10 = 1
        OCR1A = 0;              // TOP = 0 -> compare match on every timer tick
    }
    else
    {
        // Calculate number of clock cycles for the desired period
        unsigned long cycles = ((unsigned long)F_CPU * period_us) / 1000000UL;
        if (cycles < 1UL)
            cycles = 1UL;

        // Choose the smallest prescaler that allows OCR1A <= 0xFFFF
        if (cycles <= 65536UL)
        {
            clockSelectBits = 0x01; // prescaler 1 (CS10)
            OCR1A = (uint16_t)(cycles - 1UL);
        }
        else if (cycles <= 65536UL * 8UL)
        {
            clockSelectBits = 0x02; // prescaler 8 (CS11)
            OCR1A = (uint16_t)((cycles / 8UL) - 1UL);
        }
        else if (cycles <= 65536UL * 64UL)
        {
            clockSelectBits = 0x03; // prescaler 64 (CS11 | CS10)
            OCR1A = (uint16_t)((cycles / 64UL) - 1UL);
        }
        else if (cycles <= 65536UL * 256UL)
        {
            clockSelectBits = 0x04; // prescaler 256 (CS12)
            OCR1A = (uint16_t)((cycles / 256UL) - 1UL);
        }
        else
        {
            clockSelectBits = 0x05; // prescaler 1024 (CS12 | CS10)
            // Cap OCR1A at max if period is very large
            unsigned long maxCycles = 65536UL * 1024UL;
            if (cycles > maxCycles)
            {
                OCR1A = 0xFFFF;
            }
            else
            {
                OCR1A = (uint16_t)((cycles / 1024UL) - 1UL);
            }
        }
    }
}

void Timer::attachInterrupt(void (*isr)())
{
    // Atomically update the callback pointer
    uint8_t oldSREG = SREG;
    cli();
    Timer::callback = isr;
    SREG = oldSREG;
}

void Timer::start()
{
    // Reset timer count for a full period on start
    TCNT1 = 0;
    // Clear any pending Timer1 compare match flag to avoid spurious interrupts
    TIFR1 |= (1 << OCF1A);
    // Enable Timer1 Compare Match A interrupt
    TIMSK1 |= (1 << OCIE1A);
    // Start Timer1 with the selected prescaler (begin counting)
    TCCR1B |= clockSelectBits;
}

void Timer::stop()
{
    // Disable Timer1 Compare Match A interrupt
    TIMSK1 &= ~(1 << OCIE1A);
    // Stop the timer by clearing clock select bits (no clock source)
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
}

// Timer1 Compare Match A Interrupt Service Routine (ISR)
// This ISR calls the user-defined callback if it’s set.
ISR(TIMER1_COMPA_vect)
{
    if (Timer::callback)
    {
        Timer::callback(); // call the attached user function
    }
}
