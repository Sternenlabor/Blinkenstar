#pragma once

#include <Arduino.h>
#include "FECModem.h"
#if !defined(RX_NO_STORAGE)
#include "Storage.h"
#endif

// Parses legacy and alternate framed modem bytes into the same animation payload format used by Storage/display.
class ModemReceiver
{
public:
    enum DiagEvent : uint8_t {
        DIAG_EVENT_START = 0x01,
        DIAG_EVENT_PATTERN1 = 0x02,
        DIAG_EVENT_PATTERN2 = 0x04,
        DIAG_EVENT_END = 0x08,
        DIAG_EVENT_FRAME = 0x10,
    };

    void begin();
    void end();
    void process(); // call frequently from loop
    bool hasFrameComplete(); // true once after END processed
    // Diagnostics: capture the most recent decoded bytes (post-FEC)
    void getLastBytes(uint8_t *out8);
    void getStartBytes(uint8_t *out8, uint8_t &count);
    uint8_t consumeDiagEvents();
    uint16_t getDiagLength() const;
    bool isShowingHex() const;
    bool isShowingRaw() const;

private:
    enum TransmissionControl : uint8_t {
        BYTE_END = 0x84,
        // Legacy v2 markers
        BYTE_START1 = 0xa5,
        BYTE_START2 = 0x5a,
        BYTE_PATTERN1 = 0x0f,
        BYTE_PATTERN2 = 0xf0,
        // Alternate markers per MessageSpecification.md
        BYTE_START_ALT = 0x99,   // repeated twice
        BYTE_PATTERN_ALT = 0xa9, // repeated twice
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
    // Keep diagnostics intentionally small; the ATtiny88 only has 512 bytes of SRAM.
    bool frame_complete_ = false;
    uint8_t diag_last8_[8] = {0};
    uint8_t diag_idx_ = 0;
    uint8_t diag_start8_[8] = {0};
    uint8_t diag_start_count_ = 0;
    bool diag_start_capture_ = false;
    uint8_t diag_events_ = 0;
    uint16_t diag_length_ = 0;

#ifdef RX_BUFFERED_STORE
    // Buffered store: collect 32B pages in RAM, flush to EEPROM at END
    #ifndef STORE_MAX_PAGES
    #define STORE_MAX_PAGES 2
    #endif
    uint8_t store_pages_[STORE_MAX_PAGES][32];
    uint8_t store_pages_used_ = 0; // number of filled 32B pages
    uint8_t store_partial_len_ = 0; // bytes in current (incomplete) page
#endif
};

extern ModemReceiver modemReceiver;
