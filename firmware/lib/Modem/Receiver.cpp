#include "Receiver.h"
#include "DebugSerial.h"
#include "Display.h"
#include "static_patterns.h"

ModemReceiver modemReceiver;

static bool storage_ready = false;
#ifdef DIAG_RX
static uint8_t diag_hex[40];
static uint8_t diag_hex_len = 0;
static uint8_t diag_capture_count = 0;
static bool diag_showing_hex = false;
static uint8_t diag_raw[24];
static uint8_t diag_raw_len = 0;
static bool diag_showing_raw = false;
#endif

void ModemReceiver::begin()
{
    // Defer storage.enable() until a real frame starts to avoid interfering with display pins at boot
    storage_ready = false;
    fecModem.begin();
    debuglog::println("RX BEGIN");
    state_ = START1;
    rx_pos_ = 0;
    remaining_ = 0;
    diag_start_count_ = 0;
    diag_start_capture_ = false;
    diag_events_ = 0;
    diag_length_ = 0;
#ifdef DIAG_RX
    diag_hex_len = 0;
    diag_capture_count = 0;
    diag_showing_hex = false;
    diag_raw_len = 0;
    diag_showing_raw = false;
#endif
}

void ModemReceiver::end()
{
    fecModem.end();
    debuglog::println("RX END");
}

void ModemReceiver::process()
{
    // Only poll the ADC in explicit polling builds. In ISR builds this would
    // race the interrupt-driven demodulator and corrupt the receive state.
#ifdef RX_POLLING
    g_modem.poll(16);
#endif

    uint8_t budget = 32;
    while (budget-- && fecModem.available())
    {
        uint8_t b = fecModem.read();
        // Blink top-left pixel for 2 frames to indicate byte arrival
        display.setIndicator(0, 0, 2);
        // Record last 8 decoded bytes for diagnostics
        diag_last8_[diag_idx_] = b;
        diag_idx_ = (uint8_t)((diag_idx_ + 1) & 0x07);
        if (diag_start_capture_ && diag_start_count_ < 8)
        {
            diag_start8_[diag_start_count_++] = b;
            if (diag_start_count_ >= 8)
            {
                diag_start_capture_ = false;
            }
        }

#ifdef DIAG_RX
        // Show RAW bytes (pre-FEC) after START: capture 4 and display
        if (!diag_showing_raw && state_ >= NEXT_BLOCK)
        {
            uint8_t tmp[8];
            uint8_t n = g_modem.getRecentRaw(tmp, 4);
            if (n >= 4)
            {
                auto to_hex = [](uint8_t n) -> uint8_t { n &= 0x0F; return (n < 10) ? (uint8_t)('0' + n) : (uint8_t)('A' + (n - 10)); };
                uint8_t len = 0;
                diag_raw[len++] = 'R'; diag_raw[len++] = 'W'; diag_raw[len++] = ':'; diag_raw[len++] = ' ';
                for (uint8_t i = 0; i < 4; ++i)
                {
                    diag_raw[len++] = to_hex(tmp[i] >> 4);
                    diag_raw[len++] = to_hex(tmp[i]);
                    diag_raw[len++] = ' ';
                }
                diag_raw_len = len;
                animation_t a; a.type = AnimationType::TEXT; a.length = diag_raw_len; a.speed = 10; a.delay = 0; a.direction = 0; a.repeat = 0; a.data = diag_raw;
                display.show(&a);
                diag_showing_raw = true;
            }
        }
#endif

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
            // Accept either 0xA5,0x5A (legacy v2) or 0x99,0x99 (alternate)
            if (b == BYTE_START1 || b == BYTE_START_ALT)
                state_ = START2;
            break;
        case START2:
            if (b == BYTE_START2 || b == BYTE_START_ALT)
            {
                state_ = NEXT_BLOCK;
                diag_events_ |= DIAG_EVENT_START;
                diag_start_count_ = 0;
                diag_start_capture_ = true;
                // In diagnostics, no large buffering to conserve SRAM
#ifndef RX_NO_STORAGE
                if (!storage_ready)
                {
                    storage.enable();
                    storage_ready = true;
                }
                storage.reset();
#endif
                // Show a lightweight receive cue without large buffers (use both ends for visibility)
                display.clearColumns();
                display.setIndicator(0, 0, 8);
                display.setIndicator(7, 0, 8);
                display.setIndicator(0, 7, 8);
                display.setIndicator(7, 7, 8);
#ifdef DIAG_RX
                diag_hex_len = 0;
                diag_capture_count = 0;
                diag_showing_hex = false;
#endif
            }
            else
            {
                state_ = (b == BYTE_START1) ? START2 : START1;
            }
            break;
        case NEXT_BLOCK:
            // Accept either 0x0F,0xF0 or two times 0xA9 as PATTERN marker
            if (b == BYTE_PATTERN1 || b == BYTE_PATTERN_ALT)
            {
                state_ = PATTERN2;
                diag_events_ |= DIAG_EVENT_PATTERN1;
            }
            else if (b == BYTE_END)
            {
                diag_events_ |= DIAG_EVENT_END;
                // End of frame: either store or show directly (diagnostic)
#if !defined(RX_NO_STORAGE) && !defined(RX_BUFFERED_STORE)
                storage.sync();
                state_ = START1;
                fecModem.clear();
                frame_complete_ = true;
                diag_events_ |= DIAG_EVENT_FRAME;

                // Reload the just-written payload so the user sees exactly what landed in EEPROM.
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
#elif defined(RX_BUFFERED_STORE)
                // Flush buffered pages to EEPROM now
                if (!storage_ready)
                {
                    storage.enable();
                    storage_ready = true;
                }
                storage.reset();
                if (store_pages_used_ > 0)
                {
                    storage.save(store_pages_[0]);
                    for (uint8_t p = 1; p < store_pages_used_; ++p)
                    {
                        storage.append(store_pages_[p]);
                    }
                }
                storage.sync();
                state_ = START1;
                fecModem.clear();
                frame_complete_ = true;
                diag_events_ |= DIAG_EVENT_FRAME;

                // Reload the just-written payload so the display path matches the persisted bytes.
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
#else
                state_ = START1;
                fecModem.clear();
                frame_complete_ = true;
                diag_events_ |= DIAG_EVENT_FRAME;
#ifdef DIAG_RX
                // If we captured some bytes but didn't reach 12 yet, show what we have
                if (!diag_showing_hex && diag_capture_count > 0)
                {
                    animation_t anim;
                    anim.type = AnimationType::TEXT;
                    anim.length = diag_hex_len;
                    anim.speed = 8;
                    anim.delay = 0;
                    anim.direction = 0;
                    anim.repeat = 0;
                    anim.data = diag_hex;
                    display.show(&anim);
                    diag_showing_hex = true;
                }
#endif
#endif
            }
            else
            {
                // Ignore stray bytes, keep waiting for PATTERN or END
                state_ = NEXT_BLOCK;
            }
            break;
        case PATTERN2:
            if (b == BYTE_PATTERN2 || b == BYTE_PATTERN_ALT)
            {
                state_ = HEADER1;
                rx_pos_ = 0;
                diag_events_ |= DIAG_EVENT_PATTERN2;
            }
            else
            {
                // Try again: wait for NEXT_BLOCK marker sequence
                state_ = NEXT_BLOCK;
            }
            break;
        case HEADER1:
            state_ = HEADER2;
            // Header uses 12 bits of payload length split across the first two bytes.
            remaining_ = (b & 0x0F) << 8;
            break;
        case HEADER2:
            state_ = META1;
            remaining_ += b;
            diag_length_ = remaining_;
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
#if !defined(RX_NO_STORAGE) && !defined(RX_BUFFERED_STORE)
                storage.save(rx_buf_);
                display.setIndicator(0, 0, 2);
                display.setIndicator(0, 7, 2);
#endif
#ifdef RX_BUFFERED_STORE
                if (rx_pos_)
                {
                    if (store_pages_used_ < STORE_MAX_PAGES)
                    {
                        // Pad the short terminal page so EEPROM writes stay aligned to 32-byte device pages.
                        for (uint8_t i = rx_pos_; i < 32; ++i) rx_buf_[i] = 0;
                        for (uint8_t i = 0; i < 32; ++i) store_pages_[store_pages_used_][i] = rx_buf_[i];
                        store_pages_used_++;
                        store_partial_len_ = 0;
                    }
                }
#endif
            }
            else if (rx_pos_ == 32)
            {
                state_ = DATA;
                rx_pos_ = 0;
#if !defined(RX_NO_STORAGE) && !defined(RX_BUFFERED_STORE)
                storage.save(rx_buf_);
                display.setIndicator(1, 0, 2);
                display.setIndicator(1, 7, 2);
#endif
#ifdef RX_BUFFERED_STORE
                if (store_pages_used_ < STORE_MAX_PAGES)
                {
                    for (uint8_t i = 0; i < 32; ++i) store_pages_[store_pages_used_][i] = rx_buf_[i];
                    store_pages_used_++;
                    store_partial_len_ = 0;
                }
#endif
            }
            break;
        case DATA:
            if (remaining_ == 0)
            {
                state_ = NEXT_BLOCK;
#if !defined(RX_NO_STORAGE) && !defined(RX_BUFFERED_STORE)
                storage.append(rx_buf_);
                display.setIndicator(2, 0, 2);
                display.setIndicator(2, 7, 2);
#endif
            }
            else if (rx_pos_ == 32)
            {
                rx_pos_ = 0;
#if !defined(RX_NO_STORAGE) && !defined(RX_BUFFERED_STORE)
                storage.append(rx_buf_);
                display.setIndicator(3, 0, 2);
                display.setIndicator(3, 7, 2);
#endif
#ifdef RX_BUFFERED_STORE
                if (store_pages_used_ < STORE_MAX_PAGES)
                {
                    for (uint8_t i = 0; i < 32; ++i) store_pages_[store_pages_used_][i] = rx_buf_[i];
                    store_pages_used_++;
                    store_partial_len_ = 0;
                }
#endif
            }
            break;
        default:
            state_ = START1;
            break;
        }
    }
}

bool ModemReceiver::hasFrameComplete()
{
    bool v = frame_complete_;
    frame_complete_ = false;
    return v;
}

uint8_t ModemReceiver::consumeDiagEvents()
{
    uint8_t events = diag_events_;
    diag_events_ = 0;
    return events;
}

uint16_t ModemReceiver::getDiagLength() const
{
    return diag_length_;
}

bool ModemReceiver::isShowingHex() const
{
#ifdef DIAG_RX
    return diag_showing_hex;
#else
    return false;
#endif
}

bool ModemReceiver::isShowingRaw() const
{
#ifdef DIAG_RX
    return diag_showing_raw;
#else
    return false;
#endif
}

void ModemReceiver::getLastBytes(uint8_t *out8)
{
    // Return bytes in chronological order: oldest at column 0
    uint8_t idx = diag_idx_;
    for (uint8_t i = 0; i < 8; ++i)
    {
        out8[i] = diag_last8_[idx];
        idx = (uint8_t)((idx + 1) & 0x07);
    }
}

void ModemReceiver::getStartBytes(uint8_t *out8, uint8_t &count)
{
    count = diag_start_count_;
    for (uint8_t i = 0; i < 8; ++i)
    {
        out8[i] = diag_start8_[i];
    }
}
