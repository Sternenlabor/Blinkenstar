#include "System.h"
#include "DiagLog.h"
#include "Display.h"
#include "DebugSerial.h"
#include "static_patterns.h"
#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/io.h>
#ifdef ENABLE_MODEM
#include "Receiver.h"
#include "Modem.h"
#endif

System blinkenstar;

// Buttons are wired to Port C pins: PC3 (pin 26) and PC7 (pin 20)
// Use direct port access to avoid any Arduino pin mapping ambiguity
/**
 * Return whether button 1 is currently pressed.
 *
 * @returns `true` when PC3 reads low.
 */
static inline bool button1_is_low() { return (PINC & _BV(PC3)) == 0; }

/**
 * Return whether button 2 is currently pressed.
 *
 * @returns `true` when PC7 reads low.
 */
static inline bool button2_is_low() { return (PINC & _BV(PC7)) == 0; }

#if defined(ENABLE_MODEM) && !defined(RX_NO_STORAGE)
/**
 * Restart the empty-storage boot text when no stored payload can be selected.
 */
static void showEmptyStorageMessage()
{
    display.showBootMessage();
}
#endif

/**
 * Return the number of frames stored inside the shutdown animation pattern.
 *
 * @returns Shutdown frame count.
 */
static uint8_t shutdownFrameCount()
{
    const uint8_t hdr0 = pgm_read_byte(shutdownPattern);
    const uint8_t hdr1 = pgm_read_byte(shutdownPattern + 1);
    const uint16_t data_length = ((hdr0 & 0x0F) << 8) | hdr1;

    return data_length / 8;
}

/**
 * Convert the shutdown pattern speed nibble into a real frame delay in milliseconds.
 *
 * @returns Delay between shutdown frames in milliseconds.
 */
static uint16_t shutdownFrameDelayMs()
{
    const uint8_t p2 = pgm_read_byte(shutdownPattern + 2);
    const uint8_t update_threshold = 250 - ((p2 & 0x0F) << 4);
    // Match the upstream display cadence: one column every 256 us.
    const uint32_t full_refresh_us = 8UL * 256UL;
    const uint16_t delay_ms = (uint16_t)((((uint32_t)update_threshold * full_refresh_us) + 500UL) / 1000UL);

    return delay_ms == 0 ? 1 : delay_ms;
}

/**
 * Draw one shutdown animation frame directly from PROGMEM.
 *
 * @param frame Zero-based frame index.
 */
static void showShutdownFrame(uint8_t frame)
{
    const uint16_t data_offset = (uint16_t)frame * 8U;

    for (uint8_t col = 0; col < 8; ++col)
    {
        display.setColumn(col, (uint8_t)~pgm_read_byte(shutdownPattern + 4 + data_offset + col));
    }
}

/**
 * Play the full shutdown animation sequence once.
 */
static void playShutdownAnimation()
{
    const uint8_t frame_count = shutdownFrameCount();
    const uint16_t frame_delay_ms = shutdownFrameDelayMs();

    for (uint8_t frame = 0; frame < frame_count; ++frame)
    {
        showShutdownFrame(frame);
        delay(frame_delay_ms);
    }
}

#if defined(ENABLE_MODEM) && defined(RX_ALWAYS_ON) && defined(NO_BOOT_MESSAGE) && !defined(DIAG_RX)
/**
 * Show the lightweight RX standby border used by low-RAM debug builds.
 */
static void showRxStandbyCue()
{
    // Keep a visible standby pattern in low-RAM RX builds that suppress
    // the scrolling boot text. Receiver/display.show() will replace this
    // once a real frame arrives.
    display.clearColumns();
    display.setColumn(0, 0x00);
    display.setColumn(1, 0x7E);
    display.setColumn(2, 0x7E);
    display.setColumn(3, 0x7E);
    display.setColumn(4, 0x7E);
    display.setColumn(5, 0x7E);
    display.setColumn(6, 0x7E);
    display.setColumn(7, 0x00);
}
#endif

#if defined(JP1_DEBUG_SERIAL) && defined(ENABLE_MODEM) && defined(JP1_DEBUG_TONE_DIAG)
/**
 * Print the idle modem heartbeat summary over the JP1 debug logger.
 */
static void printIdleHeartbeat()
{
    debuglog::print("HB L=0x");
    debuglog::printHex16(g_modem.getActivity());
    debuglog::print(" A=0x");
    debuglog::printHex16(g_modem.getActivityAvg());
    debuglog::print(" T=");
    debuglog::println(g_modem.isTonePresent() ? "1" : "0");
}

/**
 * Print a compact summary of the most recent receive burst for JP1 capture.
 */
static void printToneSummary()
{
    // Summarize the most recent burst in a compact JP1 log line for UART capture during bench bring-up.
    debuglog::print("RX P=0x");
    debuglog::printHex16(g_modem.consumeActivityPeak());
    debuglog::print(" X=0x");
    debuglog::printHex8(g_modem.consumeTransitionCount());
    debuglog::print(" D=");
    uint8_t recent_bytes[8];
    modemReceiver.getLastBytes(recent_bytes);
    for (uint8_t i = 0; i < 8; ++i)
    {
        debuglog::printHex8(recent_bytes[i]);
    }
    debuglog::print(" S=");
    uint8_t start_bytes[8];
    uint8_t start_count = 0;
    modemReceiver.getStartBytes(start_bytes, start_count);
    debuglog::printHex8(start_count);
    debuglog::print(":");
    for (uint8_t i = 0; i < 8; ++i)
    {
        debuglog::printHex8(start_bytes[i]);
    }
    debuglog::print(" E=");
    const uint8_t events = modemReceiver.consumeDiagEvents();
    if (events == 0)
    {
        debuglog::write('-');
    }
    else
    {
        if (events & ModemReceiver::DIAG_EVENT_START)
        {
            debuglog::write('S');
        }
        if (events & ModemReceiver::DIAG_EVENT_PATTERN1)
        {
            debuglog::write('1');
        }
        if (events & ModemReceiver::DIAG_EVENT_PATTERN2)
        {
            debuglog::write('2');
        }
        if (events & ModemReceiver::DIAG_EVENT_END)
        {
            debuglog::write('E');
        }
        if (events & ModemReceiver::DIAG_EVENT_FRAME)
        {
            debuglog::write('F');
        }
    }
    const uint16_t diag_length = modemReceiver.getDiagLength();
    if (diag_length != 0)
    {
        debuglog::print(" N=0x");
        debuglog::printHex16(diag_length);
    }
    debuglog::write('\r');
    debuglog::write('\n');
}
#endif

void System::initialize()
{
    const uint8_t reset_cause = MCUSR;
    MCUSR = 0;

    // Disable watchdog via Arduino API
    wdt_disable();

    // Configure button pins with internal pull-ups on Port C (PC3, PC7)
    DDRC &= ~( _BV(PC3) | _BV(PC7) );   // inputs
    PORTC |= _BV(PC3) | _BV(PC7);       // enable pull-ups

    debuglog::begin();
    debuglog::println("BOOT");
    debuglog::print("MCUSR=0x");
    debuglog::printlnHex8(reset_cause);
    diaglog::reset(reset_cause);
#ifdef JP1_DEBUG_SKIP_MODEM
    debuglog::println("MODEM SKIP");
#endif

    // Initialize display
    display.enable();
#if defined(ENABLE_MODEM) && !defined(RX_NO_STORAGE) && !defined(DIAG_RX) && !defined(NO_STORED_PATTERN_BOOT_RESTORE)
    current_pattern_index_ = 0;
    modemReceiver.showStoredPattern(current_pattern_index_);
#endif
#if defined(ENABLE_MODEM) && defined(RX_ALWAYS_ON) && defined(NO_BOOT_MESSAGE) && !defined(DIAG_RX)
    showRxStandbyCue();
#endif
#ifdef JP1_DEBUG_HEADLESS_RX
    // Diagnostic mode: blank the matrix refresh entirely so receive bring-up
    // is not masked by LED-matrix current draw on marginal power sources.
    display.disable();
#endif
#ifdef ENABLE_MODEM
#ifdef RX_ALWAYS_ON
  #ifdef DIAG_RX
    // Diagnostic: show reset cause + heartbeat first; start modem after delay
    uint8_t mcusr = reset_cause;
    diag_reset_cause = mcusr;
    diag_boot_phase = true;
    diag_boot_end_ms = millis() + 1500;
    // Build small reset cause text
    static uint8_t rst_text[12];
    uint8_t n = 0;
    rst_text[n++] = 'R'; rst_text[n++] = 'S'; rst_text[n++] = 'T'; rst_text[n++] = ':'; rst_text[n++] = ' ';
    if (mcusr & _BV(WDRF)) { rst_text[n++] = 'W'; rst_text[n++] = 'D'; rst_text[n++] = 'T'; }
    else if (mcusr & _BV(BORF)) { rst_text[n++] = 'B'; rst_text[n++] = 'O'; rst_text[n++] = 'R'; }
    else if (mcusr & _BV(EXTRF)) { rst_text[n++] = 'E'; rst_text[n++] = 'X'; rst_text[n++] = 'T'; }
    else if (mcusr & _BV(PORF)) { rst_text[n++] = 'P'; rst_text[n++] = 'O'; rst_text[n++] = 'R'; }
    else { rst_text[n++] = 'O'; rst_text[n++] = 'K'; }
    animation_t a; a.type = AnimationType::TEXT; a.length = n; a.speed = 10; a.delay = 0; a.direction = 0; a.repeat = 0; a.data = rst_text;
    display.show(&a);
  #else
    // Store env: defer modem start slightly to let display stabilize
    modem_enabled = false;
    modem_boot_delay = 500; // number of loop ticks before enabling
  #endif
#endif
#endif

    // Enable interrupts globally
    interrupts();
}

void System::loop()
{
#if (defined(ENABLE_MODEM) && !defined(RX_NO_STORAGE)) || (defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_NO_HEARTBEAT))
    const unsigned long loop_now = millis();
#endif

#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_NO_HEARTBEAT)
#if defined(ENABLE_MODEM) && defined(JP1_DEBUG_TONE_DIAG)
    const bool tone_present = g_modem.isTonePresent();
    if (tone_present)
    {
        debug_tone_active_ = true;
    }
    else if (debug_tone_active_)
    {
        debug_tone_active_ = false;
        debug_tone_report_pending_ = true;
    }
#endif
    if (debug_heartbeat_at_ms == 0)
    {
        debug_heartbeat_at_ms = loop_now + 1000;
    }
    else if ((long)(loop_now - debug_heartbeat_at_ms) >= 0)
    {
#ifdef ENABLE_MODEM
#ifdef JP1_DEBUG_TONE_DIAG
        if (debug_tone_report_pending_)
        {
            printToneSummary();
            debug_tone_report_pending_ = false;
        }
        else if (!tone_present)
        {
            printIdleHeartbeat();
        }
#else
        printIdleHeartbeat();
#endif
#else
        debuglog::heartbeat();
#endif
        debug_heartbeat_at_ms = loop_now + 1000;
    }
#endif

    // Optional: receive-mode toggle + processing
#ifdef ENABLE_MODEM
#ifndef RX_ALWAYS_ON
    // Toggle modem/receiver on long-press of Button2 (PC7)
    if (button2_is_low())
    {
        if (btn2_hold < RECEIVE_HOLD_TICKS)
            btn2_hold++;
        else if (!btn2_latched)
        {
            // Toggle state once per long hold
            modem_enabled = !modem_enabled;
            if (modem_enabled)
            {
                modemReceiver.begin();
                display.setIndicator(7, 0, 20); // enabled
            }
            else
            {
                modemReceiver.end();
                display.setIndicator(7, 7, 20); // disabled
            }
            btn2_latched = true;
#ifndef RX_NO_STORAGE
            // A receive-mode toggle is not also a browse action on release.
            button_mask_ = BUTTON_NONE;
            button_debounce_until_ms_ = loop_now + 25;
#endif
        }
    }
    else
    {
        btn2_hold = 0;
        btn2_latched = false;
    }
#endif

    // For RX_ALWAYS_ON, enable modem after a short boot delay
#ifdef RX_ALWAYS_ON
  #ifndef DIAG_RX
    // Give the display and analog front-end a short quiet period before enabling receive mode.
#ifndef JP1_DEBUG_SKIP_MODEM
    if (!modem_enabled)
    {
        unsigned long now = millis();
        if (modem_enable_at_ms == 0)
        {
            modem_enable_at_ms = now + MODEM_BOOT_DELAY_MS;
        }
        if ((long)(now - modem_enable_at_ms) >= 0)
        {
            modem_enabled = true;
            modemReceiver.begin();
            debuglog::println("MODEM ON");
        }
    }
#endif
  #else
    // DIAG_RX: show reset text; start modem after delay, no heartbeat
    unsigned long now = millis();
    if (diag_boot_phase)
    {
        if (now >= diag_boot_end_ms)
        {
            diag_boot_phase = false;
            modem_enabled = true;
            modemReceiver.begin();
        }
    }
  #endif
#endif

#if defined(ENABLE_MODEM) && !defined(RX_NO_STORAGE)
    if ((long)(loop_now - button_debounce_until_ms_) >= 0)
    {
        if (button1_is_low())
        {
            // Match the upstream browse semantics: the PC3 button advances.
            button_mask_ |= BUTTON_NEXT;
        }
        if (button2_is_low())
        {
            // Match the upstream browse semantics: the PC7 button rewinds.
            button_mask_ |= BUTTON_PREVIOUS;
        }

        /*
         * Only browse stored patterns once the buttons are released. This
         * preserves the upstream behavior and avoids accidental flips when the
         * user actually intended a dual-button shutdown press.
         */
        if (!button1_is_low() && !button2_is_low())
        {
            storage.enable();

            if (button_mask_ == BUTTON_NEXT)
            {
                if (storage.hasData())
                {
                    current_pattern_index_ = (current_pattern_index_ + 1) % storage.numPatterns();
                    modemReceiver.showStoredPattern(current_pattern_index_);
                }
                else
                {
                    current_pattern_index_ = 0;
                    showEmptyStorageMessage();
                }
                button_debounce_until_ms_ = loop_now + 25;
            }
            else if (button_mask_ == BUTTON_PREVIOUS)
            {
                if (storage.hasData())
                {
                    if (current_pattern_index_ == 0)
                    {
                        current_pattern_index_ = storage.numPatterns() - 1;
                    }
                    else
                    {
                        current_pattern_index_--;
                    }
                    modemReceiver.showStoredPattern(current_pattern_index_);
                }
                else
                {
                    current_pattern_index_ = 0;
                    showEmptyStorageMessage();
                }
                button_debounce_until_ms_ = loop_now + 25;
            }

            button_mask_ = BUTTON_NONE;
        }
    }
#endif

    if (modem_enabled)
    {
        // Process first; if a frame completed, leave receive mode and keep the shown pattern
        modemReceiver.process();
        if (modemReceiver.hasFrameComplete())
        {
#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_NO_HEARTBEAT) && defined(JP1_DEBUG_TONE_DIAG)
            printToneSummary();
            debug_tone_report_pending_ = false;
            debug_tone_active_ = false;
#endif
            // Drop back out of receive mode once one transfer has been fully decoded and displayed.
            modem_enabled = false;
#ifndef RX_NO_STORAGE
            current_pattern_index_ = 0;
#endif
            modemReceiver.end();
            debuglog::println("FRAME DONE");
            display.setIndicator(7, 7, 20); // disabled
        }
        else
        {
            // In rxdiag, suppress live inspector to avoid bright lines; rely on RW/HX text only
            // No additional drawing here.
        }
    }
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
    DisplayState preShutdownDisplayState;

    debuglog::println("SHUTDOWN");

    display.snapshotState(preShutdownDisplayState);
    // Freeze the active image as a static frame so the timer ISR no longer
    // mutates the display while the manual shutdown animation is playing.
    display.freezeState(preShutdownDisplayState);
    playShutdownAnimation();

    // Wait until buttons are released
    while (button1_is_low() || button2_is_low())
    {
        delay(1);
    }

    // Debounce the buttons after the animation has completed.
    for (i = 0; i < 100; i++)
    {
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
    display.restoreState(preShutdownDisplayState);

    // Wait for buttons release post-wakeup
    while (button1_is_low() || button2_is_low())
    {
        delay(1);
    }

    // Debounce again
    for (i = 0; i < 100; i++)
    {
        delay(1);
    }

    // Re-enable ADC
    power_adc_enable();

    debuglog::println("WAKE");
}

/**
 * Wake the MCU from button-triggered pin-change interrupts.
 */
ISR(PCINT1_vect)
{
    // No action needed; wake-up is automatic
}
