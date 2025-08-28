#pragma once

#include <Arduino.h>
#include "Modem.h"
#include "Hamming.h"

// Wraps Modem and applies Hamming(24,16) FEC, exposing decoded bytes
class FECModem
{
public:
    void begin() { g_modem.begin(); state_ = FIRST_BYTE; }
    void end() { g_modem.end(); }

    uint8_t available(); // decoded bytes available
    uint8_t read();      // read next decoded byte
    void clear() { g_modem.clear(); state_ = FIRST_BYTE; }

private:
    enum State : uint8_t { FIRST_BYTE, SECOND_BYTE };
    State state_ = FIRST_BYTE;
    uint8_t buf_ = 0;
};

extern FECModem fecModem;
