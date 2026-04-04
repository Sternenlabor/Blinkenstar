#include "Modem.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#ifdef DIAG_RX
#include "Display.h"
#endif

Modem g_modem;

uint8_t Modem::adcDigitalInputDisableMask_()
{
    switch (MODEM_ADC_CHANNEL & 0x07)
    {
        case 0:
            return _BV(ADC0D);
        case 1:
            return _BV(ADC1D);
        case 2:
            return _BV(ADC2D);
        case 3:
            return _BV(ADC3D);
        case 4:
            return _BV(ADC4D);
        case 5:
            return _BV(ADC5D);
        case 6:
            return _BV(ADC6D);
        default:
            return _BV(ADC7D);
    }
}

void Modem::startConversion_()
{
    ADCSRA |= _BV(ADSC);
}

void Modem::processActivity_(uint16_t activity)
{
    last_activity_ = activity;
    if (activity > activity_peak_)
    {
        activity_peak_ = activity;
    }

    // Keep the adaptive envelope tracker alive for diagnostics, but use the
    // original threshold demodulator for actual bit classification.
    slicer_.update(activity);

    if (bitlen_ < 100)
        bitlen_++;

    if ((activity < ACTIVITY_THRESHOLD) && (bitlen_ > (BITLEN_THRESHOLD << 2)))
    {
        prev_freq_ = FREQ_NONE;
        bitcount_ = 0;
        byte_ = 0;
        clear();
        return;
    }

    freq_ = (activity >= ACTIVITY_THRESHOLD) ? FREQ_HIGH : FREQ_LOW;
    if (freq_ != prev_freq_)
    {
        if (transition_count_ != 0xFF)
        {
            transition_count_++;
        }
        if (prev_freq_ != FREQ_NONE)
        {
            // Each frequency transition closes one symbol; short spans map to 0, long spans map to 1.
            byte_ = (byte_ >> 1) | (bitlen_ < BITLEN_THRESHOLD ? 0x00 : 0x80);
            if (!(++bitcount_ % 8))
            {
                put_(byte_);
                // Update recent raw bytes (pre-FEC)
                recent_[recent_idx_] = byte_;
                recent_idx_ = (recent_idx_ + 1) & 0x07;
                if (recent_count_ < 8)
                    recent_count_++;
            }
        }
        prev_freq_ = freq_;
        bitlen_ = 0;
    }
}

void Modem::begin()
{
#ifndef MODEM_DISABLE_FRONTEND_BIAS
    // Bias the receive front-end while the modem is active.
    DDRA |= _BV(PA3);
    PORTA |= _BV(PA3);
#endif
    slicer_.reset();
    activity_peak_ = 0;
    transition_count_ = 0;

    // Configure ADC: AVcc reference, selectable input channel
    ADMUX = _BV(REFS0) | (MODEM_ADC_CHANNEL & 0x0F);
    // Keep the active ADC pin in analog mode only while the modem is running.
    DIDR0 |= adcDigitalInputDisableMask_();
    // Enable ADC with either free-running or manual single-shot restart.
#ifdef RX_POLLING
    // Polling: no ADC interrupt
  #ifdef RX_SLOW_ADC
    // Prescaler 128
    #ifdef MODEM_SINGLE_SHOT_ADC
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
    #else
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0) | _BV(ADATE);
    #endif
  #else
    // Prescaler 32
    #ifdef MODEM_SINGLE_SHOT_ADC
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS0);
    #else
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS0) | _BV(ADATE);
    #endif
  #endif
#else
  #ifdef RX_SLOW_ADC
    // Prescaler 128 (lower ISR rate) for coexistence with display
    #ifdef MODEM_SINGLE_SHOT_ADC
    ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
    #else
    ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0) | _BV(ADATE);
    #endif
  #else
    // Prescaler 32 (legacy-like timing)
    #ifdef MODEM_SINGLE_SHOT_ADC
    ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS0);
    #else
    ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS0) | _BV(ADATE);
    #endif
  #endif
#endif
    startConversion_();
}

void Modem::end()
{
#ifndef MODEM_DISABLE_FRONTEND_BIAS
    PORTA &= ~_BV(PA3);
    DDRA &= ~_BV(PA3);
#endif
    ADCSRA &= ~_BV(ADEN);
    DIDR0 &= ~adcDigitalInputDisableMask_();
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
    // Activity is the accumulated absolute delta across a small sample window rather than raw ADC level.
    uint16_t sample = ADC;
#ifdef MODEM_SINGLE_SHOT_ADC
    startConversion_();
#endif
    uint16_t delta = (sample > sample_prev_) ? (sample - sample_prev_) : (sample_prev_ - sample);
    sample_prev_ = sample;
    sample_accu_ += delta;
    if (++sample_cnt_ < NUMBER_OF_SAMPLES)
        return;

    uint16_t activity = sample_accu_;
    sample_cnt_ = 0;
    sample_accu_ = 0;
    processActivity_(activity);
}

ISR(ADC_vect)
{
    g_modem.onAdcIsr();
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
#ifdef MODEM_SINGLE_SHOT_ADC
        startConversion_();
#endif
        uint16_t delta = (sample > sample_prev_) ? (sample - sample_prev_) : (sample_prev_ - sample);
        sample_prev_ = sample;
        sample_accu_ += delta;
        if (++sample_cnt_ < NUMBER_OF_SAMPLES)
        {
            continue;
        }

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
        processActivity_(activity);
    }
}

uint8_t Modem::getRecentRaw(uint8_t *out, uint8_t max_n) const
{
    uint8_t n = recent_count_ < max_n ? recent_count_ : max_n;
    uint8_t start = (recent_idx_ + 8 - n) & 0x07; // oldest
    for (uint8_t i = 0; i < n; ++i)
    {
        out[i] = recent_[(start + i) & 0x07];
    }
    return n;
}
