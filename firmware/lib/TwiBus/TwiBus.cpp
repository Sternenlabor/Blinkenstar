#include "TwiBus.h"

#include <avr/io.h>
#include <util/delay.h>

TwiBus twiBus;

/**
 * Start a TWI read transaction and send the device read address.
 *
 * @param deviceAddress 7-bit target device address.
 * @returns `TwiBus::OK` on success or a bus status on failure.
 */
TwiBus::Status TwiBus::startRead_(uint8_t deviceAddress)
{
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
    while (!(TWCR & _BV(TWINT)))
    {
    }

    if (!((TWSR & 0x18) == 0x08 || (TWSR & 0x18) == 0x10))
    {
        return TwiBus::START_ERR;
    }

    TWDR = (deviceAddress << 1) | 1;
    TWCR = _BV(TWINT) | _BV(TWEN);
    while (!(TWCR & _BV(TWINT)))
    {
    }

    if (TWSR != 0x40)
    {
        return TwiBus::ADDR_ERR;
    }

    return TwiBus::OK;
}

/**
 * Start a TWI write transaction and send the device write address.
 *
 * @param deviceAddress 7-bit target device address.
 * @returns `TwiBus::OK` on success or a bus status on failure.
 */
TwiBus::Status TwiBus::startWrite_(uint8_t deviceAddress)
{
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
    while (!(TWCR & _BV(TWINT)))
    {
    }

    if (!((TWSR & 0x18) == 0x08 || (TWSR & 0x18) == 0x10))
    {
        return TwiBus::START_ERR;
    }

    TWDR = (deviceAddress << 1) | 0;
    TWCR = _BV(TWINT) | _BV(TWEN);
    while (!(TWCR & _BV(TWINT)))
    {
    }

    if (TWSR != 0x18)
    {
        return TwiBus::ADDR_ERR;
    }

    return TwiBus::OK;
}

/**
 * Send a sequence of bytes over the current TWI write transaction.
 *
 * @param len Number of bytes to send.
 * @param data Source buffer.
 * @returns Number of bytes acknowledged by the target device.
 */
uint8_t TwiBus::send_(uint8_t len, uint8_t *data)
{
    for (uint8_t pos = 0; pos < len; pos++)
    {
        TWDR = data[pos];
        TWCR = _BV(TWINT) | _BV(TWEN);
        while (!(TWCR & _BV(TWINT)))
        {
        }

        if (TWSR != 0x28)
        {
            return pos;
        }
    }

    return len;
}

/**
 * Receive a sequence of bytes from the current TWI read transaction.
 *
 * @param len Number of bytes to read.
 * @param data Destination buffer.
 * @returns Number of bytes stored in the destination buffer.
 */
uint8_t TwiBus::receive_(uint8_t len, uint8_t *data)
{
    for (uint8_t pos = 0; pos < len; pos++)
    {
        if (pos == (uint8_t)(len - 1))
        {
            TWCR = _BV(TWINT) | _BV(TWEN);
        }
        else
        {
            TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);
        }

        while (!(TWCR & _BV(TWINT)))
        {
        }

        data[pos] = TWDR;
    }

    return len;
}

/**
 * Terminate the current TWI transaction with a STOP condition.
 */
void TwiBus::stop_()
{
    TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);
}

void TwiBus::enable()
{
    /*
     * Set I2C clock frequency to 100kHz.
     * freq = F_CPU / (16 + (2 * TWBR * TWPS) )
     * let TWPS = "00" = 1
     * -> TWBR = (F_CPU / 100000) - 16 / 2
     */
    TWSR = 0; // prescaler = 1
    TWBR = ((F_CPU / 100000UL) - 16) / 2;
}

TwiBus::Status TwiBus::write(uint8_t deviceAddress, uint8_t addrhi, uint8_t addrlo, uint8_t len, uint8_t *data)
{
    uint8_t addr_buf[2];

    addr_buf[0] = addrhi;
    addr_buf[1] = addrlo;

    /*
     * The target device might be busy processing a previous write command,
     * so preserve the original retry budget and pacing during the extraction.
     */
    for (uint8_t num_tries = 0; num_tries < 32; num_tries++)
    {
        if (num_tries > 0)
        {
            _delay_us(500);
        }

        if (startWrite_(deviceAddress) != OK)
        {
            stop_();
            continue;
        }

        if (send_(2, addr_buf) != 2)
        {
            stop_();
            continue;
        }

        if (send_(len, data) != len)
        {
            stop_();
            continue;
        }

        stop_();
        return OK;
    }

    stop_();
    return DATA_ERR;
}

TwiBus::Status TwiBus::read(uint8_t deviceAddress, uint8_t addrhi, uint8_t addrlo, uint8_t len, uint8_t *data)
{
    uint8_t addr_buf[2];

    addr_buf[0] = addrhi;
    addr_buf[1] = addrlo;

    /*
     * Preserve the original combined write-address then repeated-start read
     * sequence so EEPROM access timing stays unchanged.
     */
    for (uint8_t num_tries = 0; num_tries < 32; num_tries++)
    {
        if (num_tries > 0)
        {
            _delay_us(500);
        }

        if (startWrite_(deviceAddress) != OK)
        {
            stop_();
            continue;
        }

        if (send_(2, addr_buf) != 2)
        {
            stop_();
            continue;
        }

        if (startRead_(deviceAddress) != OK)
        {
            stop_();
            continue;
        }

        if (receive_(len, data) != len)
        {
            stop_();
            continue;
        }

        stop_();
        return OK;
    }

    stop_();
    return DATA_ERR;
}
