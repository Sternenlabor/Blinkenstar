#pragma once

#include <Arduino.h>
#include "Modem.h"
#include "Hamming.h"

// Wraps Modem and applies Hamming(24,16) FEC, exposing decoded bytes
class FECModem
{
public:
    /**
     * Start the raw modem and reset the FEC byte-pair state machine.
     */
    void begin() { g_modem.begin(); state_ = FIRST_BYTE; }

    /**
     * Stop the underlying raw modem.
     */
    void end() { g_modem.end(); }

    /**
     * Report how many decoded payload bytes are ready.
     *
     * @returns Number of decoded bytes available to read.
     */
    uint8_t available();

    /**
     * Read the next decoded payload byte.
     *
     * @returns Decoded payload byte.
     */
    uint8_t read();

    /**
     * Drop any buffered raw bytes and restart FEC pair assembly.
     */
    void clear() { g_modem.clear(); state_ = FIRST_BYTE; }

private:
    enum State : uint8_t { FIRST_BYTE, SECOND_BYTE };
    State state_ = FIRST_BYTE;
    uint8_t buf_ = 0;
};

extern FECModem fecModem;
