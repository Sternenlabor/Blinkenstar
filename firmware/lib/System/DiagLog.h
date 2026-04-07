#pragma once

#include <Arduino.h>

namespace diaglog
{
/**
 * Reset the internal diagnostic log and record the reset cause.
 *
 * @param resetCause Raw MCUSR reset-cause value.
 */
#ifdef DIAG_INTERNAL_LOG
void reset(uint8_t resetCause);
#else
inline void reset(uint8_t) {}
#endif

/**
 * Record the current high-level diagnostic state byte.
 *
 * @param state State marker to store.
 */
#ifdef DIAG_INTERNAL_LOG
void setState(uint8_t state);
#else
inline void setState(uint8_t) {}
#endif

/**
 * Increment the diagnostic counter for detected frame starts.
 */
#ifdef DIAG_INTERNAL_LOG
void markStart();
#else
inline void markStart() {}
#endif

/**
 * Increment the diagnostic counter for the first pattern marker.
 */
#ifdef DIAG_INTERNAL_LOG
void markPattern1();
#else
inline void markPattern1() {}
#endif

/**
 * Increment the diagnostic counter for the second pattern marker.
 */
#ifdef DIAG_INTERNAL_LOG
void markPattern2();
#else
inline void markPattern2() {}
#endif

/**
 * Increment the diagnostic counter for end markers.
 */
#ifdef DIAG_INTERNAL_LOG
void markEnd();
#else
inline void markEnd() {}
#endif

/**
 * Increment the diagnostic counter for completed frames.
 */
#ifdef DIAG_INTERNAL_LOG
void markFrame();
#else
inline void markFrame() {}
#endif

/**
 * Record the most recent decoded payload length.
 *
 * @param length Payload length in bytes.
 */
#ifdef DIAG_INTERNAL_LOG
void setLength(uint16_t length);
#else
inline void setLength(uint16_t) {}
#endif

/**
 * Capture the first received 32-byte page for post-mortem inspection.
 *
 * @param page32 Pointer to the page buffer.
 */
#ifdef DIAG_INTERNAL_LOG
void captureFirstPage(const uint8_t *page32);
#else
inline void captureFirstPage(const uint8_t *) {}
#endif

/**
 * Increment the diagnostic counter for storage save operations.
 */
#ifdef DIAG_INTERNAL_LOG
void markSave();
#else
inline void markSave() {}
#endif

/**
 * Increment the diagnostic counter for storage append operations.
 */
#ifdef DIAG_INTERNAL_LOG
void markAppend();
#else
inline void markAppend() {}
#endif

/**
 * Capture the payload bytes read back from storage after a receive completes.
 *
 * @param payload Pointer to the loaded payload buffer.
 * @param numPatterns Number of stored patterns reported by storage.
 * @param shown Whether the payload was accepted for display.
 */
#ifdef DIAG_INTERNAL_LOG
void captureLoaded(const uint8_t *payload, uint8_t numPatterns, bool shown);
#else
inline void captureLoaded(const uint8_t *, uint8_t, bool) {}
#endif
}
