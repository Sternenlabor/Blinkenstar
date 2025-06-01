/*
 * Copyright (C) 2016 by Birte Kristina Friesel
 *
 * License: You may use, redistribute and/or modify this file under the terms
 * of either:
 * * The GNU LGPL v3 (see COPYING and COPYING.LESSER), or
 * * The 3-clause BSD License (see COPYING.BSD)
 */

#include <Arduino.h>
#include <Wire.h>
#include "Storage.h"

Storage storage;

/*
 * EEPROM data structure ("file system"):
 *
 * Organized as 32B-pages, all animations/texts are page-aligned.  Byte 0 ..
 * 255 : storage metadata. Byte 0 contains the number of animations, byte 1 the
 * page offset of the first animation, byte 2 of the second, and so on.
 * Byte 256+: texts/animations without additional storage metadata, aligned
 * to 32B. So, a maximum of 256-(256/32) = 248 texts/animations can be stored,
 * and a maximum of 255 * 32 = 8160 Bytes (almost 8 kB / 64 kbit) can be
 * addressed.  To support larger EEPROMS, change the metadate area to Byte 2 ..
 * 511 and use 16bit page pointers.
 *
 * The text/animation size is not limited by this approach.
 *
 * Example:
 * Byte     0 = 3 -> we've got a total of three animations
 * Byte     1 = 0 -> first text/animation starts at byte 256 + 32*0 = 256
 * Byte     2 = 4 -> second starts at byte 256 + 32*4 = 384
 * Byte     3 = 5 -> third starts at 256 + 32*5 * 416
 * Byte     4 = whatever
 *            .
 *            .
 *            .
 * Byte 256ff = first text/animation. Has a header encoding its length in bytes.
 * Byte 384ff = second
 * Byte 416ff = third
 *            .
 *            .
 *            .
 */

void Storage::enable()
{
    /*
     * Set I2C clock frequency to 100kHz.
     * freq = F_CPU / (16 + (2 * TWBR * TWPS) )
     * let TWPS = "00" = 1
     * -> TWBR = (F_CPU / 100000) - 16 / 2
     */
    TWSR = 0; // the lower two bits control TWPS
    TWBR = ((F_CPU / 100000UL) - 16) / 2;
    Wire.begin();

    i2c_read(0, 0, 1, &num_anims);
}

void Storage::reset()
{
    first_free_page = 0;
    num_anims = 0;
}

void Storage::sync()
{
    i2c_write(0, 0, 1, &num_anims);
}

bool Storage::hasData()
{
    // Unprogrammed EEPROM pages always read 0xff
    if (num_anims == 0xff)
    {
        return false;
    }
    return num_anims;
}

void Storage::load(uint8_t idx, uint8_t *data)
{
    i2c_read(0, 1 + idx, 1, &page_offset);

    /*
     * Unconditionally read 132 bytes. The data buffer must hold at least
     * 132 bytes anyways, and this way we can save one I2C transaction. If
     * there is any speed penalty cause by this I wasn't able to notice it.
     * Also note that the EEPROM automatically wraps around when the end of
     * memory is reached, so this edge case doesn't need to be accounted for.
     */
    i2c_read(1 + (page_offset / 8), (page_offset % 8) * 32, 132, data);
}

void Storage::loadChunk(uint8_t chunk, uint8_t *data)
{
    uint8_t this_page_offset = page_offset + (4 * chunk);

    // Note that we do not load headers here -> 128 instead of 132 bytes
    i2c_read(1 + (this_page_offset / 8), (this_page_offset % 8) * 32 + 4, 128, data);
}

void Storage::save(uint8_t *data)
{
    /*
     * Technically, we can store up to 255 patterns. However, Allowing
     * 255 patterns (-> num_anims = 0xff) means we can't easily
     * distinguish between an EEPROM with 255 patterns and a factory-new
     * EEPROM (which just reads 0xff everywhere). So only 254 patterns
     * are allowed.
     */
    if (num_anims < 254)
    {
        /*
         * Bytes 0 .. 255 (pages 0 .. 7) are reserved for storage metadata,
         * first_free_page counts pages starting at byte 256 (page 8).
         * So, first_free_page == 247 addresses EEPROM bytes 8160 .. 8191.
         * first_free_page == 248 would address bytes 8192 and up, which don't
         * exist -> don't save anything afterwards.
         *
         * Note that at the moment (stored patterns are aligned to page
         * boundaries) this means we can actually only store up to 248
         * patterns.
         */
        if (first_free_page < 248)
        {
            num_anims++;
            i2c_write(0, num_anims, 1, &first_free_page);
            append(data);
        }
    }
}

void Storage::append(uint8_t *data)
{
    // see comment in Storage::save()
    if (first_free_page < 248)
    {
        // the header indicates the length of the data, but we really don't care
        // - it's easier to just write the whole page and skip the trailing
        // garbage when reading.
        i2c_write(1 + (first_free_page / 8), (first_free_page % 8) * 32, 32, data);
        first_free_page++;
    }
}

uint8_t Storage::i2c_write(uint8_t addrhi, uint8_t addrlo, uint8_t len, uint8_t *data)
{
    /*
     * The EEPROM might be busy processing a write command, which can
     * take up to 10ms. Wait up to 16ms to respond before giving up.
     * All other error conditions (even though they should never happen[tm])
     * are handled the same way.
     */
    for (uint8_t num_tries = 0; num_tries < 32; num_tries++)
    {
        if (num_tries > 0)
        {
            delayMicroseconds(500);
        }

        Wire.beginTransmission(I2C_EEPROM_ADDR);
        Wire.write(addrhi);
        Wire.write(addrlo);
        Wire.write(data, len);

        if (Wire.endTransmission() == 0)
        {
            return I2C_OK;
        }
    }
    return I2C_ERR;
}

uint8_t Storage::i2c_read(uint8_t addrhi, uint8_t addrlo, uint8_t len, uint8_t *data)
{
    /*
     * See comments in i2c_write.
     */
    for (uint8_t num_tries = 0; num_tries < 32; num_tries++)
    {
        if (num_tries > 0)
        {
            delayMicroseconds(500);
        }

        Wire.beginTransmission(I2C_EEPROM_ADDR);
        Wire.write(addrhi);
        Wire.write(addrlo);

        if (Wire.endTransmission(false) != 0)
        { // Restart for read
            continue;
        }

        uint8_t received = Wire.requestFrom(I2C_EEPROM_ADDR, len);
        if (received != len)
        {
            continue;
        }

        for (uint8_t i = 0; i < len; i++)
        {
            data[i] = Wire.read();
        }
        return I2C_OK;
    }
    return I2C_ERR;
}