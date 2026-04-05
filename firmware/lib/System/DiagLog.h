#pragma once

#include <Arduino.h>

namespace diaglog
{
/**
 * Reset the internal diagnostic log and record the reset cause.
 *
 * @param resetCause Raw MCUSR reset-cause value.
 */
void reset(uint8_t resetCause);

/**
 * Record the current high-level diagnostic state byte.
 *
 * @param state State marker to store.
 */
void setState(uint8_t state);

/**
 * Increment the diagnostic counter for detected frame starts.
 */
void markStart();

/**
 * Increment the diagnostic counter for the first pattern marker.
 */
void markPattern1();

/**
 * Increment the diagnostic counter for the second pattern marker.
 */
void markPattern2();

/**
 * Increment the diagnostic counter for end markers.
 */
void markEnd();

/**
 * Increment the diagnostic counter for completed frames.
 */
void markFrame();

/**
 * Record the most recent decoded payload length.
 *
 * @param length Payload length in bytes.
 */
void setLength(uint16_t length);

/**
 * Capture the first received 32-byte page for post-mortem inspection.
 *
 * @param page32 Pointer to the page buffer.
 */
void captureFirstPage(const uint8_t *page32);

/**
 * Increment the diagnostic counter for storage save operations.
 */
void markSave();

/**
 * Increment the diagnostic counter for storage append operations.
 */
void markAppend();

/**
 * Capture the payload bytes read back from storage after a receive completes.
 *
 * @param payload Pointer to the loaded payload buffer.
 * @param numPatterns Number of stored patterns reported by storage.
 * @param shown Whether the payload was accepted for display.
 */
void captureLoaded(const uint8_t *payload, uint8_t numPatterns, bool shown);
}
