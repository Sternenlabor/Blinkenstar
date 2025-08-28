#pragma once

#include <Arduino.h>
#include <avr/pgmspace.h>

// Arduino-style Hamming helper (12,8) and (24,16)
class Hamming
{
public:
    enum Result : uint8_t
    {
        NO_ERROR = 0,
        ERROR_IN_PARITY = 0xfe,
        UNCORRECTABLE = 0xff,
    };

    static uint8_t parity128(uint8_t byte);
    static uint8_t parity2416(uint8_t byte1, uint8_t byte2);
    static uint8_t correct128(uint8_t &byte, uint8_t parity);
    static uint8_t correct2416(uint8_t &byte1, uint8_t &byte2, uint8_t parity);

private:
    static const uint8_t parityLow[] PROGMEM;
    static const uint8_t parityHigh[] PROGMEM;
    static const uint8_t parityCheck[] PROGMEM;
};

