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

    /**
     * Compute the Hamming parity nibble for one payload byte.
     *
     * @param byte Payload byte to protect.
     * @returns Encoded parity nibble.
     */
    static uint8_t parity128(uint8_t byte);

    /**
     * Compute the packed parity byte for a pair of payload bytes.
     *
     * @param byte1 First payload byte.
     * @param byte2 Second payload byte.
     * @returns Packed parity byte.
     */
    static uint8_t parity2416(uint8_t byte1, uint8_t byte2);

    /**
     * Correct a single-byte payload using its parity nibble.
     *
     * @param byte Payload byte to validate and possibly correct.
     * @param parity Parity nibble for the byte.
     * @returns Error count classification for the correction attempt.
     */
    static uint8_t correct128(uint8_t &byte, uint8_t parity);

    /**
     * Correct a two-byte payload using the packed parity byte.
     *
     * @param byte1 First payload byte to validate and possibly correct.
     * @param byte2 Second payload byte to validate and possibly correct.
     * @param parity Packed parity byte.
     * @returns Combined correction/error count for both bytes.
     */
    static uint8_t correct2416(uint8_t &byte1, uint8_t &byte2, uint8_t parity);

private:
    static const uint8_t parityLow[] PROGMEM;
    static const uint8_t parityHigh[] PROGMEM;
    static const uint8_t parityCheck[] PROGMEM;
};
