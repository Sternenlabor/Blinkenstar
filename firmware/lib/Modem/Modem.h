#pragma once

#include <Arduino.h>

// Receive-only audio modem using ADC free-running mode and simple FSK-like detection
class Modem
{
public:
    Modem() {}

    void begin();   // enable ADC sampling and internal state
    void end();     // disable ADC

    // ring buffer API (raw received bytes)
    uint8_t available() const;
    uint8_t read();
    void clear();

    // ISR hook
    void onAdcIsr();

    // Polling alternative to ISR to reduce contention with display
    void poll(uint8_t budget = 64);

private:
    // ring buffer (size must be power of 2)
    static constexpr uint8_t BUF_SIZE = 64;
    volatile uint8_t head_ = 0;
    volatile uint8_t tail_ = 0;
    uint8_t buf_[BUF_SIZE];

    inline void put_(uint8_t b)
    {
        uint8_t next = head_ + 1;
        if ((uint8_t)(next - tail_) != BUF_SIZE)
        {
            buf_[head_ & (BUF_SIZE - 1)] = b;
            head_ = next;
        }
    }

    // Demodulation state
    static constexpr uint8_t FREQ_NONE = 0;
    static constexpr uint8_t FREQ_LOW = 1;
    static constexpr uint8_t FREQ_HIGH = 2;

    // Allow tuning via build flags
    #ifndef MODEM_NUMBER_OF_SAMPLES
    #define MODEM_NUMBER_OF_SAMPLES 8
    #endif
    #ifndef MODEM_ACTIVITY_THRESHOLD
    #define MODEM_ACTIVITY_THRESHOLD 150
    #endif
    #ifndef MODEM_BITLEN_THRESHOLD
    #define MODEM_BITLEN_THRESHOLD 6
    #endif

    static constexpr uint8_t NUMBER_OF_SAMPLES = MODEM_NUMBER_OF_SAMPLES;
    static constexpr uint16_t ACTIVITY_THRESHOLD = MODEM_ACTIVITY_THRESHOLD;
    static constexpr uint8_t BITLEN_THRESHOLD = MODEM_BITLEN_THRESHOLD;

    uint8_t bitcount_ = 0;
    uint8_t byte_ = 0;

    uint8_t sample_cnt_ = 0;
    uint16_t sample_prev_ = 512;
    uint16_t sample_accu_ = 0;
    uint8_t bitlen_ = 0;
    uint8_t freq_ = FREQ_NONE;
    uint8_t prev_freq_ = FREQ_NONE;
};

extern Modem g_modem;

// Allow board-specific override for ADC channel (default 6 = PA0/ADC6)
#ifndef MODEM_ADC_CHANNEL
#define MODEM_ADC_CHANNEL 6
#endif
