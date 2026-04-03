# Blinkenstar

![Alt text](Blinkenstar.png?raw=true "Blinkenstar")

![Alt text](Blinkenstar_front.png?raw=true "Blinkenstar Front")

![Alt text](Blinkenstar_back.png?raw=true "Blinkenstar Back")

## Hardware

This repository contains schematics, board layout and the BOM

## Firmware Builds

The firmware now uses four named PlatformIO environments:

- `release`: normal RX/store firmware for the board
- `debugwire`: the same RX/store firmware, built for debugWIRE
- `jp1debug`: RX diagnostic firmware with TX-only JP1 serial logs
- `hwdiag`: button/display hardware diagnostic firmware

JP1 serial wiring for `jp1debug`:

- Build: `PLATFORMIO_CORE_DIR=/tmp/pio-core ~/.platformio/penv/bin/pio run -e jp1debug`
- `JP1 pin 2` (`E1`, `PC0`) is debug TX
- `JP1 pin 4` is GND
- Connect a USB-UART adapter `RX` to `JP1 pin 2`
- The `jp1debug` build emits modem activity heartbeats and RX state logs

## Audio Test Tone

Install the Node dependency once:

```bash
npm install
```

Start the continuous 1 kHz sine wave:

```bash
npm run tone:test
```

The tone plays through the default audio output until you stop it with `Ctrl+C`.

This sine source only verifies the analog path into `PA0`. It does not produce
modem frames by itself, so the receiver will not decode `START`/`PATTERN`
markers from this signal alone.

## Audio Transfer Test

Play a one-shot transfer payload that mirrors the live editor upload logic:

```bash
npm run transfer:test
```

This sends a recognizable text pattern once and exits. The payload uses the
same dual-format transfer framing as the live site, which makes it suitable for
bench validation of the actual upload path rather than just analog tone
presence.
