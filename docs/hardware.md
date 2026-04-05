# Hardware Guide

## Source Of Truth

The hardware source files live in [`hardware/Blinkenstar/`](../hardware/Blinkenstar/).
Treat these as the editable source of truth:

- [`Blinkenstar.kicad_sch`](../hardware/Blinkenstar/Blinkenstar.kicad_sch)
- [`Blinkenstar.kicad_pcb`](../hardware/Blinkenstar/Blinkenstar.kicad_pcb)
- [`Blinkenstar.kicad_pro`](../hardware/Blinkenstar/Blinkenstar.kicad_pro)
- [`sym-lib-table`](../hardware/Blinkenstar/sym-lib-table)
- [`fp-lib-table`](../hardware/Blinkenstar/fp-lib-table)
- [`Library.pretty/`](../hardware/Blinkenstar/Library.pretty/)

## Generated Or Snapshot Artifacts

These directories contain generated outputs, release snapshots, or historical backups:

- [`gerber/`](../hardware/Blinkenstar/gerber/)
- [`production/`](../hardware/Blinkenstar/production/)
- [`bom/`](../hardware/Blinkenstar/bom/)
- [`Blinkenstar-backups/`](../hardware/Blinkenstar/Blinkenstar-backups/)

Do not hand-edit those files unless the task is specifically about inspecting an exported artifact.
Regenerate them from the KiCad sources when a hardware release is prepared.

## Supporting Assets

Other useful hardware-side directories:

- [`graphics/`](../hardware/Blinkenstar/graphics/)
  Artwork and SVG assets used around the KiCad project.
- [`parts/`](../hardware/Blinkenstar/parts/)
  Local symbols, footprints, and `STEP` models for board components.

## Manufacturing Outputs

The checked-in production data includes things such as:

- zipped fabrication bundles
- BOM exports
- placement files
- IPC netlists
- drill and Gerber outputs

Keep those aligned with the KiCad source revision they were generated from.

## Practical Rule

If a hardware change is needed:

1. edit the KiCad source files
2. review the resulting schematic and PCB changes there
3. regenerate fabrication outputs if the change is intended for release

Do not patch generated fabrication files by hand.
