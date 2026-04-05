#include <Arduino.h>

#define I2C_EEPROM_ADDR 0x50

class Storage
{
private:
    /**
     * Number of animations on the storage, AKA contents of byte 0x0000.
     * A value of 0xff indicates that the EEPROM was never written to
     * and therefore contains no animations.
     */
    uint8_t num_anims;

    /**
     * Page offset of the pattern read by the last load() call. Used to
     * calculate the read address in loadChunk(). The animation this
     * offset refers to starts at byte 256 + (32 * page_offset).
     */
    uint8_t page_offset;

    /**
     * First free page (excluding the 256 byte metadata area) on the
     * EEPROM. Used by save() and append(). This value refers to the
     * EEPROM bytes 256 + (32 * first_free_page) and up.
     */
    uint8_t first_free_page;

public:
    /**
     * Construct an empty storage facade before the EEPROM is queried.
     */
    Storage()
    {
        num_anims = 0;
        first_free_page = 0;
    }

    /**
     * Enables the storage hardware: Configures the internal I2C
     * module and reads num_anims from the EEPROM.
     */
    void enable();

    /**
     * Prepares the storage for a complete overwrite by setting the
     * number of stored animations to zero. The next save operation
     * will get pattern id 0 and overwrite the first stored pattern.
     *
     * Note that this function does not write anything to the
     * EEPROM. Use Storage::sync() for that.
     */
    void reset();

    /**
     * Writes the current number of animations (as set by reset() or
     * save() to the EEPROM. Required to get a consistent storage state
     * after a power cycle.
     */
    void sync();

    /**
     * Checks whether the EEPROM contains animation data.
     *
     * @return true if the EEPROM contains valid-looking data
     */
    bool hasData();

    /**
     * Accessor for the number of saved patterns on the EEPROM.
     * A return value of 255 (0xff) means that there are no patterns
     * on the EEPROM (and hasData() returns false in that case).
     *
     * @return number of patterns
     */
    uint8_t numPatterns()
    {
        return num_anims;
    }

    /**
     * Loads pattern number idx from the EEPROM.
     *
     * @param idx pattern index (starting with 0)
     * @param data pointer to the data structure for the pattern. Must be
     *        at least 132 bytes
     */
    void load(uint8_t idx, uint8_t *data);

    /**
     * Load partial pattern chunk (without header) from EEPROM.
     *
     * @param chunk 128 byte-offset inside pattern (starting with 0)
     * @param data pointer to data structure for the pattern. Must be
     *        at least 128 bytes
     */
    void loadChunk(uint8_t chunk, uint8_t *data);

    /**
     * Save (possibly partial) pattern on the EEPROM. 32 bytes of
     * pattern data will be read and stored, regardless of the
     * pattern header.
     *
     * @param data pattern data. Must be at least 32 bytes
     */
    void save(uint8_t *data);

    /**
     * Continue saving a pattern on the EEPROM. Appends 32 bytes of
     * pattern data after the most recently written block of data
     * (i.e., to the pattern which is currently being saved).
     *
     * @param data pattern data. Must be at least 32 bytes
     */
    void append(uint8_t *data);
};

extern Storage storage;
