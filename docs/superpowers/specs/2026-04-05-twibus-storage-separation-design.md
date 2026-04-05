# TwiBus Storage Separation Design

## Goal

Separate the generic AVR TWI/I2C transaction logic from `Storage` so `Storage` only owns EEPROM layout and pattern persistence, while a new `TwiBus` library owns bus setup and byte transport.

## Current State

`Storage` currently mixes two concerns:

- EEPROM-specific behavior:
  - animation metadata layout
  - page alignment and address math
  - save/load/append semantics
- Generic TWI bus behavior:
  - `TWBR`/`TWCR` register configuration
  - start/restart/stop sequencing
  - byte send/receive helpers
  - retry and pacing logic

That makes the storage layer harder to reason about and harder to reuse if another TWI peripheral is added later.

## Recommended Design

Introduce a dedicated `TwiBus` library under `firmware/lib/TwiBus/` and move all bus-level functionality there.

### Responsibilities

`TwiBus` will own:

- TWI initialization for the ATtiny88 hardware peripheral
- bus status enum for start/address/data failures
- STOP generation
- low-level helper routines for start-read, start-write, send, and receive
- high-level `read()` and `write()` transaction helpers
- the existing retry window and `_delay_us(500)` pacing

`Storage` will keep:

- EEPROM device address constant
- metadata layout semantics
- page offset and free-page tracking
- load/loadChunk/save/append/reset/sync behavior
- EEPROM-specific address calculations and size assumptions

## API Shape

### New Library

Files:

- `firmware/lib/TwiBus/TwiBus.h`
- `firmware/lib/TwiBus/TwiBus.cpp`

Planned interface:

- `enum Status : uint8_t`
- `void enable()`
- `uint8_t read(uint8_t deviceAddress, uint8_t addrhi, uint8_t addrlo, uint8_t len, uint8_t *data)`
- `uint8_t write(uint8_t deviceAddress, uint8_t addrhi, uint8_t addrlo, uint8_t len, uint8_t *data)`

There will also be one global bus instance:

- `extern TwiBus twiBus`

This matches the current code style, which already uses global singletons like `storage`, `display`, `fecModem`, and `modemReceiver`.

### Updated Storage Layer

`Storage` will stop owning:

- `I2CStatus`
- `i2c_read(...)`
- `i2c_write(...)`

Instead it will call the new bus object with the EEPROM device address.

The EEPROM address constant will stay in `Storage`, because it is device-specific rather than bus-specific.

## Behavioral Constraints

This refactor must preserve all current behavior:

- same 100 kHz TWI configuration
- same EEPROM retry count of `32`
- same `_delay_us(500)` retry pacing
- same start/address/data status handling
- same storage metadata layout
- same page alignment behavior
- same SRAM and flash behavior as much as practical

No protocol changes, storage format changes, or receive-path behavior changes are intended.

## Testing Plan

Refactor with test-first coverage around the boundary change.

### Test updates

- Move the retry/pacing assertions from `test/storage-eeprom-write-timing.test.mjs` so they target `firmware/lib/TwiBus/TwiBus.cpp`.
- Add a test that `Storage.cpp` no longer contains raw `TWBR`, `TWCR`, `TWDR`, or `TWSR` register usage.
- Add a test that `Storage.h` no longer declares `i2c_read` or `i2c_write`.

### Verification

After implementation:

- `npm test`
- `pio run -e release`
- `pio run -e jp1debug`
- `pio run -e hwdiag`

`debugwire` should be checked too, but it is already known to overflow flash in the current tree, so it is not a clean regression signal for this refactor.

## Risks And Mitigations

### Risk: silent storage regression

Because the EEPROM path is timing-sensitive, the main risk is changing retry or transaction ordering by accident.

Mitigation:

- preserve helper logic nearly verbatim during extraction
- move tests to the new bus file instead of rewriting expectations
- keep the EEPROM address math in `Storage` only

### Risk: over-abstraction

A too-generic bus API would add churn without value.

Mitigation:

- keep `TwiBus` narrowly focused on current 16-bit-address transaction needs
- do not introduce device wrapper classes or templated abstractions

## Files Expected To Change

- `firmware/lib/Storage/Storage.h`
- `firmware/lib/Storage/Storage.cpp`
- `firmware/lib/TwiBus/TwiBus.h`
- `firmware/lib/TwiBus/TwiBus.cpp`
- `test/storage-eeprom-write-timing.test.mjs`
- one or more new/updated storage boundary regression tests

## Success Criteria

- `Storage` no longer contains general TWI transaction code
- `TwiBus` owns the reusable AVR TWI bus logic
- all existing storage behavior remains intact
- repo tests and relevant firmware builds pass
