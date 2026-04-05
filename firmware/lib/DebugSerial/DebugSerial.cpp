#include "DebugSerial.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#ifndef JP1_DEBUG_BAUD
#define JP1_DEBUG_BAUD 9600UL
#endif

#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_SILENT)
namespace
{
constexpr uint8_t kTxBit = PC0;
constexpr uint16_t kBitDelayUs = static_cast<uint16_t>(1000000UL / JP1_DEBUG_BAUD);

/**
 * Drive the JP1 TX line high for idle or data bits.
 */
inline void txHigh()
{
    PORTC |= _BV(kTxBit);
}

/**
 * Drive the JP1 TX line low for start bits or zero bits.
 */
inline void txLow()
{
    PORTC &= (uint8_t)~_BV(kTxBit);
}

/**
 * Wait exactly one software UART bit period.
 */
inline void delayBit()
{
    _delay_us(kBitDelayUs);
}

/**
 * Print one hexadecimal nibble as an ASCII character.
 *
 * @param nibble Nibble to print.
 */
inline void writeHexNibble(uint8_t nibble)
{
    if (nibble < 10)
    {
        debuglog::write((uint8_t)('0' + nibble));
    }
    else
    {
        debuglog::write((uint8_t)('A' + (nibble - 10)));
    }
}
} // namespace
#endif

void debuglog::begin()
{
#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_SILENT)
    // JP1 pin 2 is wired as a TX-only debug output for simple USB-UART capture during receiver bring-up.
    DDRC |= _BV(kTxBit);
    txHigh();
#endif
}

void debuglog::write(uint8_t value)
{
#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_SILENT)
    const uint8_t oldSreg = SREG;
    cli();

    txLow();
    delayBit();

    for (uint8_t bit = 0; bit < 8; ++bit)
    {
        if (value & 0x01)
        {
            txHigh();
        }
        else
        {
            txLow();
        }
        delayBit();
        value >>= 1;
    }

    txHigh();
    delayBit();

    SREG = oldSreg;
#else
    (void)value;
#endif
}

void debuglog::print(const char *text)
{
    if (!text)
    {
        return;
    }

    while (*text)
    {
        write((uint8_t)*text++);
    }
}

void debuglog::println(const char *text)
{
    print(text);
    write('\r');
    write('\n');
}

void debuglog::printHex8(uint8_t value)
{
#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_SILENT)
    writeHexNibble((uint8_t)(value >> 4));
    writeHexNibble((uint8_t)(value & 0x0F));
#else
    (void)value;
#endif
}

void debuglog::printlnHex8(uint8_t value)
{
    printHex8(value);
    write('\r');
    write('\n');
}

void debuglog::printHex16(uint16_t value)
{
#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_SILENT)
    writeHexNibble((uint8_t)((value >> 12) & 0x0F));
    writeHexNibble((uint8_t)((value >> 8) & 0x0F));
    writeHexNibble((uint8_t)((value >> 4) & 0x0F));
    writeHexNibble((uint8_t)(value & 0x0F));
#else
    (void)value;
#endif
}

void debuglog::printlnHex16(uint16_t value)
{
    printHex16(value);
    write('\r');
    write('\n');
}

void debuglog::heartbeat()
{
    println("HB");
}
