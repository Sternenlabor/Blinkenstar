#ifndef DEBUGSERIAL_H
#define DEBUGSERIAL_H

#include <stdint.h>

namespace debuglog
{
/**
 * Configure the JP1 debug TX pin when serial diagnostics are enabled.
 */
void begin();

/**
 * Send one raw byte over the JP1 debug serial output.
 *
 * @param value Byte to transmit.
 */
void write(uint8_t value);

/**
 * Send a null-terminated string without a trailing newline.
 *
 * @param text Text to transmit.
 */
void print(const char *text);

/**
 * Send a null-terminated string followed by CRLF.
 *
 * @param text Text to transmit.
 */
void println(const char *text);

/**
 * Send an 8-bit value as two hexadecimal digits.
 *
 * @param value Byte value to print.
 */
void printHex8(uint8_t value);

/**
 * Send an 8-bit value as hexadecimal followed by CRLF.
 *
 * @param value Byte value to print.
 */
void printlnHex8(uint8_t value);

/**
 * Send a 16-bit value as four hexadecimal digits.
 *
 * @param value Word value to print.
 */
void printHex16(uint16_t value);

/**
 * Send a 16-bit value as hexadecimal followed by CRLF.
 *
 * @param value Word value to print.
 */
void printlnHex16(uint16_t value);

/**
 * Emit the minimal periodic heartbeat string used by the JP1 diagnostics.
 */
void heartbeat();
} // namespace debuglog

#endif
