#include "Hamming.h"

const uint8_t Hamming::parityLow[] PROGMEM =
    {0, 3, 5, 6, 6, 5, 3, 0, 7, 4, 2, 1, 1, 2, 4, 7};

const uint8_t Hamming::parityHigh[] PROGMEM =
    {0, 9, 10, 3, 11, 2, 1, 8, 12, 5, 6, 15, 7, 14, 13, 4};

const uint8_t Hamming::parityCheck[] PROGMEM = {
    NO_ERROR,         // 0x0
    ERROR_IN_PARITY,  // 0x1
    ERROR_IN_PARITY,  // 0x2
    0x01,             // 0x3
    ERROR_IN_PARITY,  // 0x4
    0x02,             // 0x5
    0x04,             // 0x6
    0x08,             // 0x7
    ERROR_IN_PARITY,  // 0x8
    0x10,             // 0x9
    0x20,             // 0xA
    0x40,             // 0xB
    0x80,             // 0xC
    UNCORRECTABLE,    // 0xD
    UNCORRECTABLE,    // 0xE
    UNCORRECTABLE     // 0xF
};

uint8_t Hamming::parity128(uint8_t byte)
{
    return pgm_read_byte(&parityLow[byte & 0x0F]) ^ pgm_read_byte(&parityHigh[byte >> 4]);
}

uint8_t Hamming::parity2416(uint8_t byte1, uint8_t byte2)
{
    return parity128(byte1) | (parity128(byte2) << 4);
}

uint8_t Hamming::correct128(uint8_t &byte, uint8_t parity)
{
    uint8_t result = pgm_read_byte(&parityCheck[parity & 0x0F]);
    if (result == NO_ERROR)
        return 0;
    if (result == UNCORRECTABLE)
    {
        byte = 0; // signal error to caller
        return 3;
    }
    if (result != ERROR_IN_PARITY)
    {
        byte ^= result;
    }
    return 1;
}

uint8_t Hamming::correct2416(uint8_t &byte1, uint8_t &byte2, uint8_t parity)
{
    uint8_t err = parity2416(byte1, byte2) ^ parity;
    if (!err)
        return 0;
    return correct128(byte1, err) + correct128(byte2, err >> 4);
}

