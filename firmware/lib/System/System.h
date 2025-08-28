#ifndef SYSTEM_H
#define SYSTEM_H

#include <Arduino.h>

#define SHUTDOWN_THRESHOLD 2048
// Number of consecutive polls with both buttons low required
// before starting to count toward shutdown threshold
#define BOTH_STABLE_THRESHOLD 32

class System
{
public:
    void initialize();
    void loop();
    void shutdown();

private:
    uint16_t want_shutdown = 0; // Track long-press duration
    uint16_t both_pressed_stable = 0; // Stability gate for simultaneous press
};

extern System rocket;

#endif
