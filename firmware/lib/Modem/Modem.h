#pragma once

#include <Arduino.h>
#include "ActivitySlicer.h"

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

    // Diagnostics: expose recent activity (sum of absolute deltas)
    uint16_t getActivity() const { return last_activity_; }
    uint16_t getActivityAvg() const { return slicer_.average(); }
    uint16_t consumeActivityPeak()
    {
        uint16_t peak = activity_peak_;
        activity_peak_ = last_activity_;
        return peak;
    }
    uint8_t consumeTransitionCount()
    {
        uint8_t count = transition_count_;
        transition_count_ = 0;
        return count;
    }
    bool isTonePresent() const { return slicer_.tonePresent(); }

    // Diagnostics: capture recent raw bytes (pre-FEC)
    void clearRecentRaw() { recent_count_ = 0; }
    uint8_t getRecentRaw(uint8_t *out, uint8_t max_n) const;

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
    #ifndef MODEM_TONE_DIAG_ON_THRESHOLD
    #define MODEM_TONE_DIAG_ON_THRESHOLD 12
    #endif
    #ifndef MODEM_TONE_DIAG_OFF_THRESHOLD
    #define MODEM_TONE_DIAG_OFF_THRESHOLD 8
    #endif
    #ifndef MODEM_ACTIVITY_SPAN_THRESHOLD
    #define MODEM_ACTIVITY_SPAN_THRESHOLD 24
    #endif

    static constexpr uint8_t NUMBER_OF_SAMPLES = MODEM_NUMBER_OF_SAMPLES;
    static constexpr uint16_t ACTIVITY_THRESHOLD = MODEM_ACTIVITY_THRESHOLD;
    static constexpr uint8_t BITLEN_THRESHOLD = MODEM_BITLEN_THRESHOLD;
    static constexpr uint16_t TONE_DIAG_ON_THRESHOLD = MODEM_TONE_DIAG_ON_THRESHOLD;
    static constexpr uint16_t TONE_DIAG_OFF_THRESHOLD = MODEM_TONE_DIAG_OFF_THRESHOLD;
    static constexpr uint16_t ACTIVITY_SPAN_THRESHOLD = MODEM_ACTIVITY_SPAN_THRESHOLD;

    uint8_t bitcount_ = 0;
    uint8_t byte_ = 0;

    uint8_t sample_cnt_ = 0;
    uint16_t sample_prev_ = 512;
    uint16_t sample_accu_ = 0;
    uint8_t bitlen_ = 0;
    uint8_t freq_ = FREQ_NONE;
    uint8_t prev_freq_ = FREQ_NONE;

    // Diagnostics state
    uint16_t last_activity_ = 0;
    uint16_t activity_peak_ = 0;
    uint8_t transition_count_ = 0;
    ActivitySlicer<ACTIVITY_THRESHOLD, TONE_DIAG_ON_THRESHOLD, TONE_DIAG_OFF_THRESHOLD, ACTIVITY_SPAN_THRESHOLD> slicer_;

    // Recent raw bytes ring (pre-FEC), newest at rec_idx_-1
    uint8_t recent_[8] = {0};
    uint8_t recent_idx_ = 0;
    uint8_t recent_count_ = 0;

    void processActivity_(uint16_t activity);
};

extern Modem g_modem;

// Allow board-specific override for ADC channel (default 6 = PA0/ADC6)
#ifndef MODEM_ADC_CHANNEL
#define MODEM_ADC_CHANNEL 6
#endif
