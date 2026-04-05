# Blinkenstar Docs

This directory collects the practical project documentation that sits next to the source tree.
The design and implementation records in [`docs/superpowers/`](./superpowers/) remain the historical change log for scoped work.

## Guides

- [Getting Started](./getting-started.md)
  Setup, install, build, and first-run commands.
- [Firmware Guide](./firmware.md)
  PlatformIO environments, runtime architecture, and firmware-specific caveats.
- [Board Usage](./board-usage.md)
  What the board does on power-up, during transfer, during browse, during shutdown, and during factory reset.
- [Bench And Debug](./bench-and-debug.md)
  Audio test scripts, JP1 logging, and hardware diagnostics.
- [Hardware Guide](./hardware.md)
  KiCad source files, generated artifacts, and manufacturing outputs.
- [Development Guide](./development.md)
  Test strategy, repo layout, and recommended edit/build workflows.

## Quick Start

```bash
npm install
npm test
cd firmware
pio run -e release
```

For normal board usage, start with the [Board Usage](./board-usage.md) guide.
For bench bring-up and transfer validation, start with [Bench And Debug](./bench-and-debug.md).
