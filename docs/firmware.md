# Firmware Guide

## Supported Environments

The checked-in public firmware environments are:

- `release`
  Normal receive/store firmware for the board.
- `debugwire`
  The same runtime with debugWIRE-oriented settings for bring-up.
- `jp1debug`
  RX diagnostic firmware with JP1 serial logging and reduced display/storage features to save SRAM.
- `hwdiag`
  Button and display hardware diagnostic image.

There is also an internal `diaglog` environment in [`platformio.ini`](../firmware/platformio.ini) for targeted bring-up work.
Treat it as temporary engineering tooling, not as a stable user-facing image.

## Build And Upload

Build:

```bash
cd firmware
pio run -e release
```

Upload with an Atmel-ICE:

```bash
cd firmware
pio run -e release -t upload
```

## Runtime Architecture

The firmware is split into small modules in [`firmware/lib/`](../firmware/lib/):

- `System`
  Owns startup, button policy, shutdown/wake, storage browsing, and factory-reset behavior.
- `Display`
  Owns LED matrix multiplexing, text scrolling, frame playback, and display state restore.
- `Modem`
  Owns ADC sampling and the raw demodulator.
- `FECModem`
  Owns the FEC/Hamming-facing byte pipeline above the raw modem.
- `Receiver`
  Owns framed transfer parsing, storage writes, and receive-time user feedback.
- `Storage`
  Owns the EEPROM layout and pattern/page semantics.
- `TwiBus`
  Owns the generic AVR TWI/I2C transaction layer shared by storage.
- `DebugSerial`
  Owns the optional JP1 debug logger.
- `DiagLog`
  Owns the internal EEPROM-backed receive diagnostics used during bring-up.
- `Timer`
  Owns the display refresh timer plumbing.

## Important Runtime Behavior

In `release`:

- receive mode is always on
- stored content is reloaded at boot when available
- the board shows the empty-storage boot message when EEPROM has no saved content
- a valid transfer start shows the upstream-style flashing receive animation
- a completed transfer is written to external EEPROM, then shown on the matrix
- left and right buttons browse stored patterns
- holding both buttons shuts the board down
- holding both buttons during power connection clears storage

In `jp1debug`:

- boot text is suppressed to save RAM
- storage is disabled
- RX uses polling mode
- debug output is sent through JP1 TX

## Current Caveat

`debugwire` is present in the config, but it currently does not fit within the ATtiny88 flash budget in the checked-in tree.
Use `release`, `jp1debug`, or `hwdiag` for normal work unless you are explicitly trimming the debug image.
