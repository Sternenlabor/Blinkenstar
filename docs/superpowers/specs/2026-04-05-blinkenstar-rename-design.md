# Blinkenstar Full Rename Design

## Goal

Remove the remaining `rocket` and `Blinkenrocket` naming from the repository and replace it with `blinkenstar` or `Blinkenstar`, including firmware symbols, documentation, KiCad library identifiers, local KiCad asset filenames, and historical references.

## Current State

The repository already uses `Blinkenstar` as the project and hardware name in many places, but several older identifiers still remain:

- firmware runtime symbol names:
  - global `rocket` instance in the `System` module
  - `rocket.initialize()` / `rocket.loop()` call sites
- textual references:
  - `Blinkenrocket` in licensing and provenance text
- KiCad identifiers:
  - symbol library names like `blinkenrocket_cr2032-eagle-import`
  - footprint library references like `blinkenrocket_cr2032:*`
  - local asset filenames derived from the older project name

This leaves the repo internally inconsistent and forces future work to remember both names.

## Recommended Design

Perform a coordinated full rename in one pass so first-party code, docs, and KiCad sources all converge on `Blinkenstar`.

### Rename Scope

The rename will include:

- firmware variable and symbol names:
  - `rocket` -> `blinkenstar`
- documentation and prose:
  - `Blinkenrocket` -> `Blinkenstar`
  - `blinkenrocket` -> `blinkenstar`
- KiCad source-of-truth identifiers:
  - symbol library names
  - `lib_id` references
  - footprint library references
  - local symbol-library filenames that still embed `blinkenrocket`

The rename will intentionally rewrite historical and provenance references too, per the chosen scope.

### KiCad Handling

The KiCad part of the rename must stay internally consistent:

- rename local library filenames under `hardware/Blinkenstar/`
- update `sym-lib-table` URIs to the renamed files
- update `Blinkenstar.kicad_sch` symbol-library and footprint references to the renamed library identifiers

Generated fabrication artifacts, BOM exports, and production ZIPs remain generated artifacts. They should not be hand-edited as part of this rename.

## Behavioral Constraints

This is a naming cleanup only. It must not change behavior.

Required non-goals:

- no firmware logic changes
- no protocol changes
- no display or modem behavior changes
- no EEPROM/storage format changes
- no hardware connectivity changes

## Testing Plan

Drive the implementation with structural regression coverage first.

### New Or Updated Tests

- add a test that the firmware no longer exports or references `rocket`
- add a test that first-party text files no longer contain `Blinkenrocket` or `blinkenrocket`
- add a test that KiCad source files no longer reference the old library identifiers

### Verification

After implementation:

- `npm test`
- `pio run -e release`
- `pio run -e jp1debug`
- `pio run -e hwdiag`

`debugwire` should be checked as a secondary signal, but it already has a known flash-capacity problem in the current tree and is not a clean pass/fail gate for this rename.

## Risks And Mitigations

### Risk: breaking KiCad local library references

KiCad source files refer to local library names and file paths, so a partial rename could leave the schematic unable to resolve symbols or footprints.

Mitigation:

- rename the asset filenames and KiCad references together
- add structural tests for the old identifiers in the checked-in source files

### Risk: renaming generated or third-party artifacts by accident

A broad string replacement could touch files that are snapshots rather than editable sources.

Mitigation:

- limit source edits to first-party code, docs, tests, and KiCad source-of-truth files
- leave generated fabrication outputs untouched

### Risk: inconsistent casing

The repo uses both symbol-style lowercase identifiers and human-facing title-case names.

Mitigation:

- use `blinkenstar` for code identifiers and filenames where lowercase is already the convention
- use `Blinkenstar` for prose and project names

## Files Expected To Change

- `firmware/lib/System/System.h`
- `firmware/lib/System/System.cpp`
- `firmware/src/main.cpp`
- `LICENSE`
- one or more tests under `test/`
- `hardware/Blinkenstar/sym-lib-table`
- `hardware/Blinkenstar/Blinkenstar.kicad_sch`
- one or more local KiCad library files whose filenames still include `blinkenrocket`

## Success Criteria

- no remaining `rocket` runtime symbol names in first-party firmware code
- no remaining `Blinkenrocket` or `blinkenrocket` text in first-party source files covered by the rename
- KiCad source files resolve the renamed local library identifiers consistently
- repo tests and relevant firmware builds still pass
