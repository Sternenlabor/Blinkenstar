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

## Maintained Flash Helpers

The maintained flashing helpers live at [`firmware/dist/flash.sh`](../firmware/dist/flash.sh) and [`firmware/dist/flash.bat`](../firmware/dist/flash.bat).

They replace the legacy helper path and do two things:

1. select an existing locale-tagged `.hex` from [`firmware/dist/<env>/`](../firmware/dist/)
2. flash it and write the expected ATtiny88 fuse values

They do not build firmware or copy artifacts into `firmware/dist/`.

Example:

```bash
cd firmware
./dist/flash.sh release
./dist/flash.sh release de
```

On Windows from `cmd.exe`:

```bat
cd firmware
dist\flash.bat release
dist\flash.bat release de
```

Defaults:

- environment: `release`
- locale: `en`
- MCU: `attiny88`
- programmer: `atmelice_isp`
- port: `usb`

You can override the programmer-related settings, or the `avrdude` binary path, with environment variables such as `AVRDUDE_BIN`, `PROGRAMMER`, `PORT`, or `MCU`.

Artifact selection:

- English artifacts should be present as `firmware_en.hex`
- German artifacts should be present as `firmware_de.hex`
- the helpers look under `firmware/dist/<env>/` and stop with an error if the expected file is missing

## Fuse Note

The flash helper also writes the expected fuse values during programming:

- `lfuse = 0xee`
- `hfuse = 0xdf`
- `efuse = 0xff`

That note previously lived in the legacy `_old/README.md`; it is now maintained here with the current flashing workflow.

## Runtime Architecture

The firmware is split into small modules in [`firmware/lib/`](../firmware/lib/):

- `System`
  Owns startup, button policy, shutdown/wake, storage browsing, and factory-reset behavior.
- `Display`
  Owns LED matrix multiplexing, text scrolling, frame playback, end-of-animation pause/repeat handling, and display state restore.
- `Modem`
  Owns ADC sampling and the raw demodulator.
- `FECModem`
  Owns the FEC/Hamming-facing byte pipeline above the raw modem.
- `Receiver`
  Owns framed transfer parsing, storage writes, receive-time user feedback, and interrupted-transfer timeout recovery.
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
- if a started transfer stalls for about four seconds, the receiver drops the partial frame and shows the built-in transmission-error text
- a completed transfer is written to external EEPROM, then shown on the matrix
- left and right buttons browse stored patterns
- finite-repeat stored animations now honor their encoded pause and repeat count before auto-advancing to the next stored pattern
- holding both buttons shuts the board down
- holding both buttons during power connection clears storage

In `jp1debug`:

- boot text is suppressed to save RAM
- storage is disabled
- RX uses polling mode
- debug output is sent through JP1 TX

## Intentional Upstream Difference

Upstream `blinkenrocket/firmware` puts the MCU into idle sleep between normal loop iterations so timer and modem interrupts wake it again immediately.

Blinkenstar does not currently do that during normal runtime. The checked-in firmware only enters deep sleep for the explicit shutdown path, and otherwise stays in the Arduino `loop()` model.

This is intentional for now:

- the board already has a deliberate modem boot delay to give the display and analog front-end a quiet startup window
- the current firmware goals have prioritized receive/display stability and predictable bring-up over active-idle power tuning

If this is revisited later, the likely implementation target is a bench-validated idle-sleep path in normal runtime only, gated by:

- confirming timer and modem wakeups still behave reliably on this board
- checking that receive stability is not degraded by the sleep/wake cadence
- measuring whether the power reduction is large enough to justify the added runtime complexity

## Current Caveat

`debugwire` is present in the config, but it currently does not fit within the ATtiny88 flash budget in the checked-in tree.
Use `release`, `jp1debug`, or `hwdiag` for normal work unless you are explicitly trimming the debug image.
