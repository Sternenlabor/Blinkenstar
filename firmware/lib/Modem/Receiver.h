#pragma once

#include <Arduino.h>
#include "FECModem.h"
#include "Storage.h"

// Parses incoming modem bytes and writes complete patterns to Storage
class ModemReceiver
{
public:
    void begin();
    void end();
    void process(); // call frequently from loop

private:
    enum TransmissionControl : uint8_t {
        BYTE_END = 0x84,
        BYTE_START1 = 0xa5,
        BYTE_START2 = 0x5a,
        BYTE_PATTERN1 = 0x0f,
        BYTE_PATTERN2 = 0xf0,
    };

    enum RxExpect : uint8_t {
        START1,
        START2,
        NEXT_BLOCK,
        PATTERN1,
        PATTERN2,
        HEADER1,
        HEADER2,
        META1,
        META2,
        DATA_FIRSTBLOCK,
        DATA,
    };

    RxExpect state_ = START1;
    uint8_t rx_buf_[32];
    uint8_t rx_pos_ = 0;
    uint16_t remaining_ = 0;
};

extern ModemReceiver modemReceiver;
