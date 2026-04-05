# Getting Started

## Purpose

Blinkenstar is a small AVR-based board with:

- an `8x8` LED matrix
- two front buttons
- an audio input path for transfer reception
- external EEPROM storage for received texts and animations

This repository contains the board hardware, the firmware, and Node-based bench helpers used to exercise the transfer path from a workstation.

## Prerequisites

- Node.js and npm
- PlatformIO CLI
- an AVR programmer for upload, typically an Atmel-ICE via ISP
- optional USB UART adapter for `jp1debug`
- optional audio output path for `tone:test` and `transfer:test`

## Install

Install the Node dependency once at the repo root:

```bash
npm install
```

## First Verification

Run the host-side tests:

```bash
npm test
```

Build the main firmware:

```bash
cd firmware
pio run -e release
```

## Common Commands

From the repo root:

```bash
npm test
npm run tone:test
npm run transfer:test
```

From [`firmware/`](../firmware/):

```bash
pio run -e release
pio run -e release -t upload
pio run -e jp1debug
pio run -e hwdiag
```

## Repository Layout

- [`firmware/src/`](../firmware/src/) contains the application entry point.
- [`firmware/lib/`](../firmware/lib/) contains the private firmware modules.
- [`firmware/platformio.ini`](../firmware/platformio.ini) defines the checked-in build environments.
- [`scripts/`](../scripts/) contains thin Node entry points for bench playback.
- [`scripts/lib/`](../scripts/lib/) contains reusable audio and transfer helpers.
- [`test/`](../test/) contains Node-based behavior and structural regressions.
- [`hardware/Blinkenstar/`](../hardware/Blinkenstar/) contains the KiCad project and hardware release assets.
- [`docs/superpowers/`](./superpowers/) contains design notes and implementation records for scoped changes.

## Where To Go Next

- Use [Firmware Guide](./firmware.md) for environment selection and architecture.
- Use [Board Usage](./board-usage.md) for runtime behavior and controls.
- Use [Bench And Debug](./bench-and-debug.md) for tone generation, transfer playback, and JP1 diagnostics.
