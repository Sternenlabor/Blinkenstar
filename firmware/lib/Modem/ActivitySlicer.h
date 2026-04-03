#pragma once

#include <stdint.h>

template <uint16_t IdleThreshold,
          uint16_t ToneOnThreshold,
          uint16_t ToneOffThreshold,
          uint16_t MinSpan>
class ActivitySlicer
{
public:
    enum State : uint8_t
    {
        STATE_NONE = 0,
        STATE_LOW = 1,
        STATE_HIGH = 2,
    };

    void reset()
    {
        average_ = 0;
        high_ = 0;
        low_ = 0;
        midpoint_ = 0;
        span_ = 0;
        state_ = STATE_NONE;
        tone_present_ = false;
        seeded_ = false;
    }

    State update(uint16_t activity)
    {
        // Track a smoothed envelope so tone presence does not flap with single-sample spikes.
        average_ = (uint16_t)(((uint32_t)average_ * 7u + activity) >> 3);

        if (tone_present_)
        {
            if (average_ <= ToneOffThreshold)
            {
                tone_present_ = false;
            }
        }
        else if (average_ >= ToneOnThreshold)
        {
            tone_present_ = true;
        }

        if (!tone_present_ || average_ < IdleThreshold)
        {
            seeded_ = false;
            high_ = activity;
            low_ = activity;
            midpoint_ = activity;
            span_ = 0;
            state_ = STATE_NONE;
            return state_;
        }

        if (!seeded_)
        {
            high_ = activity;
            low_ = activity;
            midpoint_ = activity;
            span_ = 0;
            seeded_ = true;
            state_ = STATE_NONE;
            return state_;
        }

        if (activity > high_)
        {
            high_ = activity;
        }
        else
        {
            high_ = stepToward_(high_, activity);
        }

        if (activity < low_)
        {
            low_ = activity;
        }
        else
        {
            low_ = stepToward_(low_, activity);
        }

        span_ = (high_ >= low_) ? (uint16_t)(high_ - low_) : 0;
        midpoint_ = low_ + (span_ >> 1);
        if (span_ < MinSpan)
        {
            state_ = STATE_NONE;
            return state_;
        }

        // Use span-relative hysteresis so weak but valid tones still toggle without chattering.
        uint16_t hysteresis = span_ >> 3;
        if (hysteresis < 4)
        {
            hysteresis = 4;
        }

        switch (state_)
        {
        case STATE_HIGH:
            if ((uint16_t)(activity + hysteresis) < midpoint_)
            {
                state_ = STATE_LOW;
            }
            break;
        case STATE_LOW:
            if (activity > (uint16_t)(midpoint_ + hysteresis))
            {
                state_ = STATE_HIGH;
            }
            break;
        case STATE_NONE:
        default:
            state_ = (activity >= midpoint_) ? STATE_HIGH : STATE_LOW;
            break;
        }

        return state_;
    }

    uint16_t average() const { return average_; }
    bool tonePresent() const { return tone_present_; }
    uint16_t midpoint() const { return midpoint_; }
    uint16_t span() const { return span_; }

private:
    static uint16_t stepToward_(uint16_t current, uint16_t target)
    {
        if (current == target)
        {
            return current;
        }

        uint16_t diff = (current > target) ? (uint16_t)(current - target) : (uint16_t)(target - current);
        uint16_t step = diff >> 2;
        if (step == 0)
        {
            step = 1;
        }

        if (current > target)
        {
            current -= step;
            return (current < target) ? target : current;
        }

        current += step;
        return (current > target) ? target : current;
    }

    uint16_t average_ = 0;
    uint16_t high_ = 0;
    uint16_t low_ = 0;
    uint16_t midpoint_ = 0;
    uint16_t span_ = 0;
    State state_ = STATE_NONE;
    bool tone_present_ = false;
    bool seeded_ = false;
};
