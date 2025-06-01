#ifndef SYSTEM_H
#define SYSTEM_H

#include <Arduino.h>

#define SHUTDOWN_THRESHOLD 2048

class System
{
public:
    void initialize();
    void loop();
    void shutdown();

private:
    uint16_t want_shutdown = 0; // Track long-press duration
};

extern System rocket;

#endif