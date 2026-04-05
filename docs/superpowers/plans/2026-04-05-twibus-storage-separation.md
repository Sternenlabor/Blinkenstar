# TwiBus Storage Separation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extract the generic AVR TWI/I2C transaction logic from `Storage` into a reusable `TwiBus` library while preserving EEPROM behavior.

**Architecture:** `TwiBus` will own bus setup, low-level TWI helper routines, transaction-level read/write methods, and the retry/pacing policy. `Storage` will keep only EEPROM-specific layout logic, metadata management, and page-oriented save/load behavior, delegating all bus transport to the new global `twiBus` instance.

**Tech Stack:** AVR C++, PlatformIO, Node.js test suite

---

### Task 1: Lock In The Refactor Boundary With Failing Tests

**Files:**
- Modify: `test/storage-eeprom-write-timing.test.mjs`
- Create or Modify: `test/storage-bus-separation.test.mjs`
- Test: `test/storage-eeprom-write-timing.test.mjs`
- Test: `test/storage-bus-separation.test.mjs`

- [ ] **Step 1: Write the failing tests**

```js
import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const twiBusSourcePath = path.join(repoRoot, 'firmware', 'lib', 'TwiBus', 'TwiBus.cpp')
const storageHeaderPath = path.join(repoRoot, 'firmware', 'lib', 'Storage', 'Storage.h')
const storageSourcePath = path.join(repoRoot, 'firmware', 'lib', 'Storage', 'Storage.cpp')

test('twi bus keeps the original EEPROM retry window and busy-wait pacing', () => {
    const twiBusSource = fs.readFileSync(twiBusSourcePath, 'utf8')

    assert.match(twiBusSource, /for \(uint8_t num_tries = 0; num_tries < 32; num_tries\+\+\)/)
    assert.match(twiBusSource, /if \(num_tries > 0\)\s*\{\s*_delay_us\(500\);\s*\}/)
})

test('storage no longer owns raw TWI helpers or register access', () => {
    const storageHeader = fs.readFileSync(storageHeaderPath, 'utf8')
    const storageSource = fs.readFileSync(storageSourcePath, 'utf8')

    assert.doesNotMatch(storageHeader, /i2c_read\(/)
    assert.doesNotMatch(storageHeader, /i2c_write\(/)
    assert.doesNotMatch(storageSource, /\bTWBR\b|\bTWCR\b|\bTWDR\b|\bTWSR\b/)
    assert.match(storageSource, /twiBus\.enable\(\);/)
    assert.match(storageSource, /twiBus\.read\(/)
    assert.match(storageSource, /twiBus\.write\(/)
})
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `node --test test/storage-eeprom-write-timing.test.mjs test/storage-bus-separation.test.mjs`

Expected: FAIL because `firmware/lib/TwiBus/TwiBus.cpp` does not exist yet and `Storage` still declares or uses raw TWI helpers.

- [ ] **Step 3: Implement the minimal test scaffolding**

Create `test/storage-bus-separation.test.mjs` with the assertions above and update `test/storage-eeprom-write-timing.test.mjs` to point at `firmware/lib/TwiBus/TwiBus.cpp`.

- [ ] **Step 4: Run the tests again**

Run: `node --test test/storage-eeprom-write-timing.test.mjs test/storage-bus-separation.test.mjs`

Expected: FAIL because production code has not been extracted yet, but the boundary assertions now exist.

- [ ] **Step 5: Commit**

```bash
git add test/storage-eeprom-write-timing.test.mjs test/storage-bus-separation.test.mjs
git commit -m "test: lock storage twi separation boundary"
```

### Task 2: Extract The Generic TWI Bus Layer

**Files:**
- Create: `firmware/lib/TwiBus/TwiBus.h`
- Create: `firmware/lib/TwiBus/TwiBus.cpp`
- Modify: `firmware/lib/Storage/Storage.h`
- Modify: `firmware/lib/Storage/Storage.cpp`
- Test: `test/storage-eeprom-write-timing.test.mjs`
- Test: `test/storage-bus-separation.test.mjs`

- [ ] **Step 1: Write the minimal new bus interface**

```cpp
#pragma once

#include <Arduino.h>

class TwiBus
{
public:
    enum Status : uint8_t
    {
        OK,
        START_ERR,
        ADDR_ERR,
        DATA_ERR
    };

    void enable();
    uint8_t read(uint8_t deviceAddress, uint8_t addrhi, uint8_t addrlo, uint8_t len, uint8_t *data);
    uint8_t write(uint8_t deviceAddress, uint8_t addrhi, uint8_t addrlo, uint8_t len, uint8_t *data);
};

extern TwiBus twiBus;
```

- [ ] **Step 2: Move the low-level TWI helpers into `TwiBus.cpp`**

```cpp
static inline void twi_stop_()
{
    TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);
}

static uint8_t startRead_(uint8_t deviceAddress) { /* extracted from Storage */ }
static uint8_t startWrite_(uint8_t deviceAddress) { /* extracted from Storage */ }
static uint8_t send_(uint8_t len, uint8_t *data) { /* extracted from Storage */ }
static uint8_t receive_(uint8_t len, uint8_t *data) { /* extracted from Storage */ }
```

- [ ] **Step 3: Port the transaction-level `read()` and `write()` methods without changing behavior**

```cpp
uint8_t TwiBus::write(uint8_t deviceAddress, uint8_t addrhi, uint8_t addrlo, uint8_t len, uint8_t *data)
{
    uint8_t addr_buf[] = {addrhi, addrlo};
    for (uint8_t num_tries = 0; num_tries < 32; num_tries++)
    {
        if (num_tries > 0)
        {
            _delay_us(500);
        }
        /* preserve the existing transaction ordering and stop handling */
    }
    return DATA_ERR;
}
```

- [ ] **Step 4: Update `Storage` to depend on `twiBus`**

```cpp
#include "TwiBus.h"

#define I2C_EEPROM_ADDR 0x50

void Storage::enable()
{
    twiBus.enable();
    twiBus.read(I2C_EEPROM_ADDR, 0, 0, 1, &num_anims);
}
```

Also remove `Storage::I2CStatus`, `i2c_read`, and `i2c_write` from `Storage.h`.

- [ ] **Step 5: Run the focused tests**

Run: `node --test test/storage-eeprom-write-timing.test.mjs test/storage-bus-separation.test.mjs`

Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add firmware/lib/TwiBus/TwiBus.h firmware/lib/TwiBus/TwiBus.cpp firmware/lib/Storage/Storage.h firmware/lib/Storage/Storage.cpp test/storage-eeprom-write-timing.test.mjs test/storage-bus-separation.test.mjs
git commit -m "refactor: extract twi bus from storage"
```

### Task 3: Run Full Verification And Clean Up

**Files:**
- Modify: `firmware/lib/TwiBus/TwiBus.h`
- Modify: `firmware/lib/TwiBus/TwiBus.cpp`
- Modify: `firmware/lib/Storage/Storage.h`
- Modify: `firmware/lib/Storage/Storage.cpp`
- Test: `test/storage-eeprom-write-timing.test.mjs`
- Test: `test/storage-bus-separation.test.mjs`

- [ ] **Step 1: Run the full JavaScript test suite**

Run: `npm test`

Expected: PASS

- [ ] **Step 2: Build the normal firmware targets**

Run: `PLATFORMIO_CORE_DIR=/tmp/pio-core ~/.platformio/penv/bin/pio run -e release`

Expected: PASS

Run: `PLATFORMIO_CORE_DIR=/tmp/pio-core ~/.platformio/penv/bin/pio run -e jp1debug`

Expected: PASS

Run: `PLATFORMIO_CORE_DIR=/tmp/pio-core ~/.platformio/penv/bin/pio run -e hwdiag`

Expected: PASS

- [ ] **Step 3: Check the known overflow target**

Run: `PLATFORMIO_CORE_DIR=/tmp/pio-core ~/.platformio/penv/bin/pio run -e debugwire`

Expected: likely FAIL with the existing flash overflow unless unrelated code size changes happen; confirm it does not fail for a new TWI-specific reason.

- [ ] **Step 4: Refactor comments or names only if needed**

```cpp
// Keep TwiBus generic: it owns bus transport only.
// Storage remains the single owner of EEPROM layout semantics.
```

- [ ] **Step 5: Commit**

```bash
git add firmware/lib/TwiBus/TwiBus.h firmware/lib/TwiBus/TwiBus.cpp firmware/lib/Storage/Storage.h firmware/lib/Storage/Storage.cpp test/storage-eeprom-write-timing.test.mjs test/storage-bus-separation.test.mjs
git commit -m "test: verify twi bus extraction"
```
