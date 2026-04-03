#ifndef DEBUGSERIAL_H
#define DEBUGSERIAL_H

#include <stdint.h>

namespace debuglog
{
void begin();
void write(uint8_t value);
void print(const char *text);
void println(const char *text);
void printHex8(uint8_t value);
void printlnHex8(uint8_t value);
void printHex16(uint16_t value);
void printlnHex16(uint16_t value);
void heartbeat();
} // namespace debuglog

#endif
