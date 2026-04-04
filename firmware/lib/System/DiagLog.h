#pragma once

#include <Arduino.h>

namespace diaglog
{
void reset(uint8_t resetCause);
void setState(uint8_t state);
void markStart();
void markPattern1();
void markPattern2();
void markEnd();
void markFrame();
void setLength(uint16_t length);
void captureFirstPage(const uint8_t *page32);
void markSave();
void markAppend();
void captureLoaded(const uint8_t *payload, uint8_t numPatterns, bool shown);
}
