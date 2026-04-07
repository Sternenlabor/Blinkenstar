#ifndef DEBUGSERIAL_H
#define DEBUGSERIAL_H

#include <stdint.h>

namespace debuglog
{
/**
 * Configure the JP1 debug TX pin when serial diagnostics are enabled.
 */
#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_SILENT)
void begin();
#else
inline void begin() {}
#endif

/**
 * Send one raw byte over the JP1 debug serial output.
 *
 * @param value Byte to transmit.
 */
#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_SILENT)
void write(uint8_t value);
#else
inline void write(uint8_t) {}
#endif

/**
 * Send a null-terminated string without a trailing newline.
 *
 * @param text Text to transmit.
 */
#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_SILENT)
void print(const char *text);
#else
inline void print(const char *) {}
#endif

/**
 * Send a null-terminated string followed by CRLF.
 *
 * @param text Text to transmit.
 */
#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_SILENT)
void println(const char *text);
#else
inline void println(const char *) {}
#endif

/**
 * Send an 8-bit value as two hexadecimal digits.
 *
 * @param value Byte value to print.
 */
#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_SILENT)
void printHex8(uint8_t value);
#else
inline void printHex8(uint8_t) {}
#endif

/**
 * Send an 8-bit value as hexadecimal followed by CRLF.
 *
 * @param value Byte value to print.
 */
#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_SILENT)
void printlnHex8(uint8_t value);
#else
inline void printlnHex8(uint8_t) {}
#endif

/**
 * Send a 16-bit value as four hexadecimal digits.
 *
 * @param value Word value to print.
 */
#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_SILENT)
void printHex16(uint16_t value);
#else
inline void printHex16(uint16_t) {}
#endif

/**
 * Send a 16-bit value as hexadecimal followed by CRLF.
 *
 * @param value Word value to print.
 */
#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_SILENT)
void printlnHex16(uint16_t value);
#else
inline void printlnHex16(uint16_t) {}
#endif

/**
 * Emit the minimal periodic heartbeat string used by the JP1 diagnostics.
 */
#if defined(JP1_DEBUG_SERIAL) && !defined(JP1_DEBUG_SILENT)
void heartbeat();
#else
inline void heartbeat() {}
#endif
} // namespace debuglog

#endif
