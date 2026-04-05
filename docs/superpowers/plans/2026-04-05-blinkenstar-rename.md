# Blinkenstar Full Rename Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rename the remaining legacy `rocket` and `Blinkenrocket` identifiers to `blinkenstar` / `Blinkenstar` across firmware, text, and KiCad source files without changing behavior.

**Architecture:** Add one structural regression test file first so the rename is locked in before any production edits. Then rename the firmware runtime symbol and textual references, update the KiCad library identifiers in the checked-in source-of-truth files, and finish with the normal JavaScript and PlatformIO verification passes. The checked-in hardware tree does not currently contain a local `.kicad_sym` file with the legacy filename, so this plan only renames the checked-in KiCad identifiers and URI strings that still carry the old project name.

**Tech Stack:** AVR C++, Node.js tests, KiCad source files, PlatformIO

---

### Task 1: Lock In The Rename With Failing Structural Tests

**Files:**
- Create: `test/blinkenstar-rename-regressions.test.mjs`
- Test: `test/blinkenstar-rename-regressions.test.mjs`

- [ ] **Step 1: Write the failing regression test**

```js
import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const systemHeaderPath = path.join(repoRoot, 'firmware', 'lib', 'System', 'System.h')
const systemSourcePath = path.join(repoRoot, 'firmware', 'lib', 'System', 'System.cpp')
const mainSourcePath = path.join(repoRoot, 'firmware', 'src', 'main.cpp')
const licensePath = path.join(repoRoot, 'LICENSE')
const symbolTablePath = path.join(repoRoot, 'hardware', 'Blinkenstar', 'sym-lib-table')
const schematicPath = path.join(repoRoot, 'hardware', 'Blinkenstar', 'Blinkenstar.kicad_sch')

test('firmware uses blinkenstar instead of the legacy rocket runtime symbol', () => {
    const systemHeader = fs.readFileSync(systemHeaderPath, 'utf8')
    const systemSource = fs.readFileSync(systemSourcePath, 'utf8')
    const mainSource = fs.readFileSync(mainSourcePath, 'utf8')

    assert.match(systemHeader, /extern System blinkenstar;/)
    assert.match(systemSource, /System blinkenstar;/)
    assert.match(mainSource, /blinkenstar\.initialize\(\);/)
    assert.match(mainSource, /blinkenstar\.loop\(\);/)
    assert.doesNotMatch(systemHeader, /extern System rocket;/)
    assert.doesNotMatch(systemSource, /System rocket;/)
    assert.doesNotMatch(mainSource, /rocket\.(initialize|loop)\(\);/)
})

test('text and kicad sources no longer use the legacy blinkenrocket naming', () => {
    const licenseText = fs.readFileSync(licensePath, 'utf8')
    const symbolTable = fs.readFileSync(symbolTablePath, 'utf8')
    const schematic = fs.readFileSync(schematicPath, 'utf8')

    assert.doesNotMatch(licenseText, /Blinkenrocket|blinkenrocket/)
    assert.doesNotMatch(symbolTable, /blinkenrocket_cr2032/)
    assert.doesNotMatch(schematic, /blinkenrocket_cr2032/)
    assert.match(symbolTable, /blinkenstar_cr2032-eagle-import/)
    assert.match(schematic, /blinkenstar_cr2032-eagle-import/)
    assert.match(schematic, /blinkenstar_cr2032:/)
})
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `node --test test/blinkenstar-rename-regressions.test.mjs`

Expected: FAIL because `System` still exports `rocket`, `main.cpp` still calls `rocket.initialize()` / `rocket.loop()`, `LICENSE` still mentions `Blinkenrocket`, and the KiCad files still reference `blinkenrocket_cr2032`.

- [ ] **Step 3: Save the regression test exactly as written**

Create `test/blinkenstar-rename-regressions.test.mjs` with the assertions above so the rest of the rename is driven by a stable red test.

- [ ] **Step 4: Run the test again**

Run: `node --test test/blinkenstar-rename-regressions.test.mjs`

Expected: FAIL for the same legacy-name reasons, confirming the test is exercising real code and source files rather than passing accidentally.

- [ ] **Step 5: Commit**

```bash
git add test/blinkenstar-rename-regressions.test.mjs
git commit -m "test: lock blinkenstar rename boundary"
```

### Task 2: Rename The Firmware Runtime Symbol And Remaining Text References

**Files:**
- Modify: `firmware/lib/System/System.h`
- Modify: `firmware/lib/System/System.cpp`
- Modify: `firmware/src/main.cpp`
- Modify: `LICENSE`
- Test: `test/blinkenstar-rename-regressions.test.mjs`

- [ ] **Step 1: Rename the exported `System` singleton**

```cpp
class System
{
    // existing API stays unchanged
};

extern System blinkenstar;
```

```cpp
#include "System.h"

System blinkenstar;
```

- [ ] **Step 2: Update the runtime call sites in `main.cpp`**

```cpp
void setup()
{
    blinkenstar.initialize();
}

void loop()
{
    blinkenstar.loop();
}
```

- [ ] **Step 3: Rewrite the remaining text reference in `LICENSE`**

```text
Based on Blinkenstar.
```

- [ ] **Step 4: Run the focused rename regression**

Run: `node --test test/blinkenstar-rename-regressions.test.mjs`

Expected: still FAIL because the KiCad source files have not been renamed yet, but the firmware symbol assertions should now pass.

- [ ] **Step 5: Commit**

```bash
git add firmware/lib/System/System.h firmware/lib/System/System.cpp firmware/src/main.cpp LICENSE test/blinkenstar-rename-regressions.test.mjs
git commit -m "refactor: rename firmware runtime to blinkenstar"
```

### Task 3: Rename KiCad Library Identifiers In Source-Of-Truth Files

**Files:**
- Modify: `hardware/Blinkenstar/sym-lib-table`
- Modify: `hardware/Blinkenstar/Blinkenstar.kicad_sch`
- Test: `test/blinkenstar-rename-regressions.test.mjs`

- [ ] **Step 1: Rename the symbol library entry in `sym-lib-table`**

```scheme
(sym_lib_table
  (version 7)
  (lib (name "blinkenstar_cr2032-eagle-import")(type "KiCad")(uri "${KIPRJMOD}/blinkenstar_cr2032-eagle-import.kicad_sym")(options "")(descr ""))
  (lib (name "B3F-1000")(type "KiCad")(uri "${KIPRJMOD}/parts/B3F-1000.kicad_sym")(options "")(descr ""))
  (lib (name "TEPT5700")(type "KiCad")(uri "${KIPRJMOD}/parts/TEPT5700.kicad_sym")(options "")(descr ""))
)
```

This is a source rename only. Do not invent or hand-create a new symbol library asset file if the corresponding `.kicad_sym` file is not present in the checked-in tree.

- [ ] **Step 2: Rewrite the schematic library IDs and footprint library references**

```scheme
(symbol "blinkenstar_cr2032-eagle-import:AVR-ISP-6VERT"
    (property "Footprint" "blinkenstar_cr2032:AVR-ISP-6"
```

```scheme
(lib_id "blinkenstar_cr2032-eagle-import:C-EUC1206")
(lib_id "blinkenstar_cr2032-eagle-import:R-EU_R1206")
(lib_id "blinkenstar_cr2032-eagle-import:SEGMENT_8X8_ROWCATHODEM12A881UR")
```

Apply the same replacement consistently to every `blinkenrocket_cr2032-eagle-import:*` and `blinkenrocket_cr2032:*` occurrence in `hardware/Blinkenstar/Blinkenstar.kicad_sch`.

- [ ] **Step 3: Keep generated hardware artifacts untouched**

Run: `rg -n 'blinkenrocket_cr2032|Blinkenrocket|rocket' hardware/Blinkenstar/gerber hardware/Blinkenstar/production hardware/Blinkenstar/Blinkenstar-backups`

Expected: ignore any matches there during the rename because those directories are generated artifacts or snapshots and should not be hand-edited.

- [ ] **Step 4: Run the focused rename regression**

Run: `node --test test/blinkenstar-rename-regressions.test.mjs`

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add hardware/Blinkenstar/sym-lib-table hardware/Blinkenstar/Blinkenstar.kicad_sch test/blinkenstar-rename-regressions.test.mjs
git commit -m "refactor: rename blinkenstar hardware identifiers"
```

### Task 4: Run Full Verification And Final Consistency Checks

**Files:**
- Modify: `test/blinkenstar-rename-regressions.test.mjs`
- Modify: `firmware/lib/System/System.h`
- Modify: `firmware/lib/System/System.cpp`
- Modify: `firmware/src/main.cpp`
- Modify: `LICENSE`
- Modify: `hardware/Blinkenstar/sym-lib-table`
- Modify: `hardware/Blinkenstar/Blinkenstar.kicad_sch`

- [ ] **Step 1: Run the full JavaScript and structural test suite**

Run: `npm test`

Expected: PASS

- [ ] **Step 2: Build the supported firmware environments**

Run: `PLATFORMIO_CORE_DIR=/tmp/pio-core ~/.platformio/penv/bin/pio run -e release`

Expected: PASS

Run: `PLATFORMIO_CORE_DIR=/tmp/pio-core ~/.platformio/penv/bin/pio run -e jp1debug`

Expected: PASS

Run: `PLATFORMIO_CORE_DIR=/tmp/pio-core ~/.platformio/penv/bin/pio run -e hwdiag`

Expected: PASS

- [ ] **Step 3: Check the known overflow environment**

Run: `PLATFORMIO_CORE_DIR=/tmp/pio-core ~/.platformio/penv/bin/pio run -e debugwire`

Expected: likely FAIL with the existing flash overflow unless code size changes unexpectedly; confirm it does not fail because of missing rename symbols or KiCad-related changes.

- [ ] **Step 4: Run one final targeted grep for accidental legacy names in the renamed source areas**

Run: `rg -n 'rocket|Rocket|ROCKET' firmware test LICENSE hardware/Blinkenstar/sym-lib-table hardware/Blinkenstar/Blinkenstar.kicad_sch`

Expected: no matches except the rename regression test source itself, where the old-name patterns are intentionally encoded as assertions.

- [ ] **Step 5: Commit**

```bash
git add firmware/lib/System/System.h firmware/lib/System/System.cpp firmware/src/main.cpp LICENSE hardware/Blinkenstar/sym-lib-table hardware/Blinkenstar/Blinkenstar.kicad_sch test/blinkenstar-rename-regressions.test.mjs
git commit -m "test: verify blinkenstar full rename"
```
