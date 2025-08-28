#include "FECModem.h"

FECModem fecModem;

uint8_t FECModem::available()
{
    uint8_t raw = g_modem.available();
    if (raw >= 3)
        return 2;
    if (state_ == SECOND_BYTE && raw >= 2)
        return 1; // already buffered first byte, waiting for parity
    return 0;
}

uint8_t FECModem::read()
{
    if (state_ == SECOND_BYTE)
    {
        state_ = FIRST_BYTE;
        return buf_;
    }
    // fetch three raw bytes: first, second, parity
    uint8_t b1 = g_modem.read();
    buf_ = g_modem.read();
    uint8_t p = g_modem.read();
    uint8_t t1 = b1;
    uint8_t t2 = buf_;
    Hamming::correct2416(t1, t2, p);
    // return first, buffer second
    buf_ = t2;
    state_ = SECOND_BYTE;
    return t1;
}

