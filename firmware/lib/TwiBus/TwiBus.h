#pragma once

#include <Arduino.h>

/**
 * Generic AVR TWI bus helper for 16-bit-addressed peripherals.
 */
class TwiBus
{
public:
    /**
     * Transaction status codes returned by the bus helper.
     */
    enum Status : uint8_t
    {
        OK,
        START_ERR,
        ADDR_ERR,
        DATA_ERR
    };

    /**
     * Configure the AVR TWI peripheral for the shared bus speed used by the firmware.
     */
    void enable();

    /**
     * Read bytes from a 16-bit-addressed I2C/TWI device.
     *
     * @param deviceAddress 7-bit device address.
     * @param addrhi Upper device-internal address byte.
     * @param addrlo Lower device-internal address byte.
     * @param len Number of bytes to read.
     * @param data Destination buffer.
     * @returns Bus transaction status.
     */
    Status read(uint8_t deviceAddress, uint8_t addrhi, uint8_t addrlo, uint8_t len, uint8_t *data);

    /**
     * Write bytes to a 16-bit-addressed I2C/TWI device.
     *
     * @param deviceAddress 7-bit device address.
     * @param addrhi Upper device-internal address byte.
     * @param addrlo Lower device-internal address byte.
     * @param len Number of bytes to write.
     * @param data Source buffer.
     * @returns Bus transaction status.
     */
    Status write(uint8_t deviceAddress, uint8_t addrhi, uint8_t addrlo, uint8_t len, uint8_t *data);

private:
    /**
     * Terminate the current TWI transaction with a STOP condition.
     */
    static void stop_();

    /**
     * Start a TWI read transaction and send the device read address.
     *
     * @param deviceAddress 7-bit target device address.
     * @returns `OK` on success or a bus status on failure.
     */
    static Status startRead_(uint8_t deviceAddress);

    /**
     * Start a TWI write transaction and send the device write address.
     *
     * @param deviceAddress 7-bit target device address.
     * @returns `OK` on success or a bus status on failure.
     */
    static Status startWrite_(uint8_t deviceAddress);

    /**
     * Send a sequence of bytes over the current TWI write transaction.
     *
     * @param len Number of bytes to send.
     * @param data Source buffer.
     * @returns Number of bytes acknowledged by the target device.
     */
    static uint8_t send_(uint8_t len, uint8_t *data);

    /**
     * Receive a sequence of bytes from the current TWI read transaction.
     *
     * @param len Number of bytes to read.
     * @param data Destination buffer.
     * @returns Number of bytes stored in the destination buffer.
     */
    static uint8_t receive_(uint8_t len, uint8_t *data);
};

extern TwiBus twiBus;
