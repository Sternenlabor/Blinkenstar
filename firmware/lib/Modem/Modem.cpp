#include "Modem.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#ifdef DIAG_RX
#include "Display.h"
#endif

Modem g_modem;

void Modem::begin()
{
    // Configure ADC: AVcc reference, selectable input channel
    ADMUX = _BV(REFS0) | (MODEM_ADC_CHANNEL & 0x0F);
    // Enable ADC, prescaler 128 (lower rate), auto trigger (free running)
    // Use polling (no ADIE) to avoid ISR contention with display multiplexing
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0) | _BV(ADATE);
    ADCSRA |= _BV(ADSC);
}

void Modem::end()
{
    ADCSRA &= ~_BV(ADEN);
}

uint8_t Modem::available() const
{
    return head_ - tail_;
}

uint8_t Modem::read()
{
    if (available() == 0)
        return 0;
    uint8_t b = buf_[tail_ & (BUF_SIZE - 1)];
    tail_++;
    return b;
}

void Modem::clear()
{
    head_ = tail_ = 0;
}

void Modem::onAdcIsr()
{
    // Free-running ADC: fetch sample and compute absolute delta
    uint16_t sample = ADC;
    uint16_t delta = (sample > sample_prev_) ? (sample - sample_prev_) : (sample_prev_ - sample);
    sample_prev_ = sample;
    sample_accu_ += delta;
    if (++sample_cnt_ < NUMBER_OF_SAMPLES)
        return;

    uint16_t activity = sample_accu_;
    sample_cnt_ = 0;
    sample_accu_ = 0;

    if (bitlen_ < 100)
        bitlen_++;

    if ((activity < ACTIVITY_THRESHOLD) && (bitlen_ > (BITLEN_THRESHOLD << 2)))
    {
        // idle
        prev_freq_ = FREQ_NONE;
        bitcount_ = 0;
        byte_ = 0;
        clear();
        return;
    }

    freq_ = (activity >= ACTIVITY_THRESHOLD) ? FREQ_HIGH : FREQ_LOW;
    if (freq_ != prev_freq_)
    {
        if (prev_freq_ != FREQ_NONE)
        {
            // edge -> decide bit from bitlen
            byte_ = (byte_ >> 1) | (bitlen_ < BITLEN_THRESHOLD ? 0x00 : 0x80);
            if (!(++bitcount_ % 8))
            {
                put_(byte_);
            }
        }
        prev_freq_ = freq_;
        bitlen_ = 0;
    }
}

void Modem::poll(uint8_t budget)
{
    while (budget--)
    {
        if (!(ADCSRA & _BV(ADIF)))
            break; // no new sample ready
        // Clear flag by writing 1
        ADCSRA |= _BV(ADIF);
        // Process one sample (same as ISR)
        uint16_t sample = ADC;
        uint16_t delta = (sample > sample_prev_) ? (sample - sample_prev_) : (sample_prev_ - sample);
        sample_prev_ = sample;
        sample_accu_ += delta;
        if (++sample_cnt_ < NUMBER_OF_SAMPLES)
            continue;

        uint16_t activity = sample_accu_;
        sample_cnt_ = 0;
        sample_accu_ = 0;

        if (bitlen_ < 100)
            bitlen_++;

#ifdef DIAG_RX
        // Visualize input activity (top-right corner) regardless of decode
        if (activity >= (ACTIVITY_THRESHOLD / 2))
        {
            display.setIndicator(7, 0, 1);
        }
#endif

        if ((activity < ACTIVITY_THRESHOLD) && (bitlen_ > (BITLEN_THRESHOLD << 2)))
        {
            // idle
            prev_freq_ = FREQ_NONE;
            bitcount_ = 0;
            byte_ = 0;
            clear();
            continue;
        }

        freq_ = (activity >= ACTIVITY_THRESHOLD) ? FREQ_HIGH : FREQ_LOW;
        if (freq_ != prev_freq_)
        {
            if (prev_freq_ != FREQ_NONE)
            {
                // edge -> decide bit from bitlen
                byte_ = (byte_ >> 1) | (bitlen_ < BITLEN_THRESHOLD ? 0x00 : 0x80);
                if (!(++bitcount_ % 8))
                {
                    put_(byte_);
                }
            }
            prev_freq_ = freq_;
            bitlen_ = 0;
        }
    }
}
