#include "DiagLog.h"

#ifdef DIAG_INTERNAL_LOG
#include <avr/eeprom.h>

namespace
{
struct DiagLogLayout
{
    uint8_t magic;
    uint8_t version;
    uint8_t reset_cause;
    uint8_t last_state;
    uint8_t start_count;
    uint8_t pattern1_count;
    uint8_t pattern2_count;
    uint8_t end_count;
    uint8_t frame_count;
    uint8_t save_count;
    uint8_t append_count;
    uint8_t num_patterns;
    uint8_t first_hdr0;
    uint8_t first_hdr1;
    uint8_t first_meta2;
    uint8_t first_meta3;
    uint8_t loaded_hdr0;
    uint8_t loaded_hdr1;
    uint8_t loaded_meta2;
    uint8_t loaded_meta3;
    uint8_t show_ok;
    uint8_t reserved;
    uint16_t length;
};

DiagLogLayout EEMEM ee_diag_log;
constexpr uint8_t kMagic = 0xD1;
constexpr uint8_t kVersion = 0x01;

/**
 * Clear one EEPROM-backed diagnostic byte.
 *
 * @param field EEPROM field to reset.
 */
void resetField(uint8_t &field)
{
    eeprom_update_byte(&field, 0);
}

/**
 * Increment one EEPROM-backed diagnostic byte in place.
 *
 * @param field EEPROM field to increment.
 */
void incrementField(uint8_t &field)
{
    uint8_t value = eeprom_read_byte(&field);
    eeprom_update_byte(&field, static_cast<uint8_t>(value + 1));
}
} // namespace

namespace diaglog
{
void reset(uint8_t resetCause)
{
    eeprom_update_byte(&ee_diag_log.magic, kMagic);
    eeprom_update_byte(&ee_diag_log.version, kVersion);
    eeprom_update_byte(&ee_diag_log.reset_cause, resetCause);
    resetField(ee_diag_log.last_state);
    resetField(ee_diag_log.start_count);
    resetField(ee_diag_log.pattern1_count);
    resetField(ee_diag_log.pattern2_count);
    resetField(ee_diag_log.end_count);
    resetField(ee_diag_log.frame_count);
    resetField(ee_diag_log.save_count);
    resetField(ee_diag_log.append_count);
    resetField(ee_diag_log.num_patterns);
    resetField(ee_diag_log.first_hdr0);
    resetField(ee_diag_log.first_hdr1);
    resetField(ee_diag_log.first_meta2);
    resetField(ee_diag_log.first_meta3);
    resetField(ee_diag_log.loaded_hdr0);
    resetField(ee_diag_log.loaded_hdr1);
    resetField(ee_diag_log.loaded_meta2);
    resetField(ee_diag_log.loaded_meta3);
    resetField(ee_diag_log.show_ok);
    resetField(ee_diag_log.reserved);
    eeprom_update_word(&ee_diag_log.length, 0);
}

void setState(uint8_t state)
{
    eeprom_update_byte(&ee_diag_log.last_state, state);
}

void markStart()
{
    incrementField(ee_diag_log.start_count);
}

void markPattern1()
{
    incrementField(ee_diag_log.pattern1_count);
}

void markPattern2()
{
    incrementField(ee_diag_log.pattern2_count);
}

void markEnd()
{
    incrementField(ee_diag_log.end_count);
}

void markFrame()
{
    incrementField(ee_diag_log.frame_count);
}

void setLength(uint16_t length)
{
    eeprom_update_word(&ee_diag_log.length, length);
}

void captureFirstPage(const uint8_t *page32)
{
    if (!page32)
    {
        return;
    }

    eeprom_update_byte(&ee_diag_log.first_hdr0, page32[0]);
    eeprom_update_byte(&ee_diag_log.first_hdr1, page32[1]);
    eeprom_update_byte(&ee_diag_log.first_meta2, page32[2]);
    eeprom_update_byte(&ee_diag_log.first_meta3, page32[3]);
}

void markSave()
{
    incrementField(ee_diag_log.save_count);
}

void markAppend()
{
    incrementField(ee_diag_log.append_count);
}

void captureLoaded(const uint8_t *payload, uint8_t numPatterns, bool shown)
{
    if (payload)
    {
        eeprom_update_byte(&ee_diag_log.loaded_hdr0, payload[0]);
        eeprom_update_byte(&ee_diag_log.loaded_hdr1, payload[1]);
        eeprom_update_byte(&ee_diag_log.loaded_meta2, payload[2]);
        eeprom_update_byte(&ee_diag_log.loaded_meta3, payload[3]);
    }
    eeprom_update_byte(&ee_diag_log.num_patterns, numPatterns);
    eeprom_update_byte(&ee_diag_log.show_ok, shown ? 1 : 0);
}
} // namespace diaglog
#endif
