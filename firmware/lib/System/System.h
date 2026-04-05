#ifndef SYSTEM_H
#define SYSTEM_H

#include <Arduino.h>

#ifndef SHUTDOWN_THRESHOLD
#define SHUTDOWN_THRESHOLD 2048
#endif
// Number of consecutive polls with both buttons low required
// before starting to count toward shutdown threshold
#ifndef BOTH_STABLE_THRESHOLD
#define BOTH_STABLE_THRESHOLD 32
#endif
#ifndef MODEM_BOOT_DELAY_MS
#define MODEM_BOOT_DELAY_MS 1000UL
#endif

class System
{
public:
    /**
     * Initialize display, buttons, diagnostics, and optional modem subsystems.
     */
    void initialize();

    /**
     * Run one iteration of the main firmware control loop.
     */
    void loop();

    /**
     * Play the shutdown animation, enter sleep, and restore state on wake.
     */
    void shutdown();

private:
    uint16_t want_shutdown = 0; // Track long-press duration
    uint16_t both_pressed_stable = 0; // Stability gate for simultaneous press
#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_NO_HEARTBEAT)
    uint32_t debug_heartbeat_at_ms = 0;
#if defined(ENABLE_MODEM) && defined(JP1_DEBUG_TONE_DIAG)
    bool debug_tone_active_ = false;
    bool debug_tone_report_pending_ = false;
#endif
#endif
#ifdef ENABLE_MODEM
    bool modem_enabled = false;
    uint16_t btn2_hold = 0;
    #ifndef RECEIVE_HOLD_TICKS
    #define RECEIVE_HOLD_TICKS 200
    #endif
    bool btn2_latched = false; // prevents multiple toggles while held
    uint16_t modem_boot_delay = 0; // legacy tick-based delay (unused when time-based)
    uint32_t modem_enable_at_ms = 0; // time-based defer of modem start
#ifdef DIAG_RX
    bool diag_boot_phase = true;
    uint32_t diag_boot_end_ms = 0;
    uint8_t diag_reset_cause = 0;
#endif
#endif
};

extern System rocket;

#endif
