#include "Receiver.h"
#include "Display.h"

ModemReceiver modemReceiver;

static bool storage_ready = false;

void ModemReceiver::begin()
{
    // Defer storage.enable() until a real frame starts to avoid interfering with display pins at boot
    storage_ready = false;
    fecModem.begin();
    state_ = START1;
    rx_pos_ = 0;
    remaining_ = 0;
}

void ModemReceiver::end()
{
    fecModem.end();
}

void ModemReceiver::process()
{
    // Poll ADC samples first (no ISR). Limit processing per call.
    g_modem.poll(128);

    uint8_t budget = 32;
    while (budget-- && fecModem.available())
    {
        uint8_t b = fecModem.read();
        // Blink top-left pixel for 2 frames to indicate byte arrival
        display.setIndicator(0, 0, 2);

        // Store bytes by default (header/meta/data), but only after PATTERN2
        if (state_ > PATTERN2)
        {
            rx_buf_[rx_pos_++] = b;
            if (state_ > META2)
            {
                if (remaining_)
                    remaining_--;
            }
        }

        switch (state_)
        {
        case START1:
            if (b == BYTE_START1)
                state_ = START2;
            break;
        case START2:
            if (b == BYTE_START2)
            {
                state_ = NEXT_BLOCK;
                if (!storage_ready)
                {
                    storage.enable();
                    storage_ready = true;
                }
                storage.reset();
                // optional: could show flashingPattern during receive
            }
            else
            {
                state_ = (b == BYTE_START1) ? START2 : START1;
            }
            break;
        case NEXT_BLOCK:
            if (b == BYTE_PATTERN1)
            {
                state_ = PATTERN2;
            }
            else if (b == BYTE_END)
            {
                storage.sync();
                state_ = START1;
                fecModem.clear();

                // Load and show the first stored pattern as confirmation
                static uint8_t disp_buf[132];
                storage.load(0, disp_buf);

                animation_t anim;
                uint8_t hdr0 = disp_buf[0];
                uint8_t hdr1 = disp_buf[1];
                anim.type = static_cast<AnimationType>(hdr0 >> 4);
                anim.length = ((hdr0 & 0x0F) << 8) | hdr1;
                uint8_t p2 = disp_buf[2];
                uint8_t p3 = disp_buf[3];
                if (anim.type == AnimationType::TEXT)
                {
                    anim.speed = 250 - (p2 & 0xF0);
                    anim.delay = (p2 & 0x0F);
                    anim.direction = (p3 >> 4);
                    anim.repeat = (p3 & 0x0F);
                }
                else
                {
                    anim.speed = 250 - ((p2 & 0x0F) << 4);
                    anim.delay = (p3 >> 4);
                    anim.direction = 0;
                    anim.repeat = (p3 & 0x0F);
                }
                anim.data = disp_buf + 4;
                display.show(&anim);
            }
            else
            {
                state_ = START1;
            }
            break;
        case PATTERN2:
            if (b == BYTE_PATTERN2)
            {
                state_ = HEADER1;
                rx_pos_ = 0;
            }
            else
            {
                state_ = START1;
            }
            break;
        case HEADER1:
            state_ = HEADER2;
            remaining_ = (b & 0x0F) << 8;
            break;
        case HEADER2:
            state_ = META1;
            remaining_ += b;
            break;
        case META1:
            state_ = META2;
            break;
        case META2:
            state_ = DATA_FIRSTBLOCK;
            if (remaining_ == 0)
                state_ = NEXT_BLOCK;
            break;
        case DATA_FIRSTBLOCK:
            if (remaining_ == 0)
            {
                state_ = NEXT_BLOCK;
                storage.save(rx_buf_);
            }
            else if (rx_pos_ == 32)
            {
                state_ = DATA;
                rx_pos_ = 0;
                storage.save(rx_buf_);
            }
            break;
        case DATA:
            if (remaining_ == 0)
            {
                state_ = NEXT_BLOCK;
                storage.append(rx_buf_);
            }
            else if (rx_pos_ == 32)
            {
                rx_pos_ = 0;
                storage.append(rx_buf_);
            }
            break;
        default:
            state_ = START1;
            break;
        }
    }
}
