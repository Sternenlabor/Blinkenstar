#ifndef TIMER_H
#define TIMER_H

#include <Arduino.h> // Provides F_CPU and AVR register definitions

class Timer
{
public:
    /**
     * Configure Timer1 to trigger an interrupt every period_us microseconds
     * (but do not start it). Chooses an appropriate prescaler and OCR1A
     * value for the desired period.
     */
    void initialize(unsigned int period_us);

    /**
     * Attach a callback function to be called on each Timer1 tick (compare match interrupt).
     * The function must have no parameters and return void.
     */
    void attachInterrupt(void (*isr)());

    /**
     * Start Timer1 with the configured period, enabling the compare match interrupt
     * to begin periodic callbacks.
     */
    void start();

    /**
     * Stop Timer1 by disabling its interrupt and stopping the timer clock.
     */
    void stop();

    /**
     * User-defined ISR callback function pointer (called from TIMER1 COMPA ISR).
     * This is publicly accessible (like TimerOne's isrCallback) so the ISR can call it.
     */
    static void (*callback)();

private:
    // Prescaler bit settings for Timer1 (determined in initialize())
    static uint8_t clockSelectBits;
};

#endif
