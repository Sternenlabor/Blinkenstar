#include <Arduino.h>
#include "System.h"

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
