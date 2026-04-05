# Blinkenstar

![Alt text](Blinkenstar.png?raw=true "Blinkenstar")

![Alt text](Blinkenstar_front.png?raw=true "Blinkenstar Front")

![Alt text](Blinkenstar_back.png?raw=true "Blinkenstar Back")

## Overview

This repository contains the Blinkenstar hardware design, AVR firmware, and Node-based bench tools used to validate audio transfer reception and stored playback on the board.

## Docs

- [Docs Index](./docs/README.md)
- [Getting Started](./docs/getting-started.md)
- [Firmware Guide](./docs/firmware.md)
- [Board Usage](./docs/board-usage.md)
- [Bench And Debug](./docs/bench-and-debug.md)
- [Hardware Guide](./docs/hardware.md)
- [Development Guide](./docs/development.md)

## Quick Start

Install dependencies and run the default checks:

```bash
npm install
npm test
```

Build the main firmware:

```bash
cd firmware
pio run -e release
```

## Firmware Environments

The public PlatformIO environments are:

- `release`: normal RX/store firmware for the board
- `debugwire`: the same RX/store firmware, built for debugWIRE
- `jp1debug`: RX diagnostic firmware with TX-only JP1 serial logs
- `hwdiag`: button/display hardware diagnostic firmware

See [Firmware Guide](./docs/firmware.md) for the current behavior and caveats of each environment.

## Bench Commands

Continuous sine-wave audio path check:

```bash
npm run tone:test
```

One-shot framed transfer playback:

```bash
npm run transfer:test
```

See [Bench And Debug](./docs/bench-and-debug.md) for JP1 wiring, diagnostic images, and recommended bring-up order.

## JP1 Wiring

JP1 serial wiring for `jp1debug`:

- Build: `PLATFORMIO_CORE_DIR=/tmp/pio-core ~/.platformio/penv/bin/pio run -e jp1debug`
- `JP1 pin 2` (`E1`, `PC0`) is debug TX
- `JP1 pin 4` is GND
- Connect a USB-UART adapter `RX` to `JP1 pin 2`
- Connect adapter `GND` to `JP1 pin 4`
