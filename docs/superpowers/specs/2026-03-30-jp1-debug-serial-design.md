# JP1 Debug Serial Design

**Date:** 2026-03-30

**Goal:** Add a dedicated debug-only firmware build that emits TX-only serial logs on JP1 without using the ATtiny88 core's built-in `Serial` pins.

## Context

The ATTinyCore `Serial` implementation for the ATtiny88 uses `AIN0/AIN1` on `PD6/PD7`. In this project those pins are part of the display row bus and are continuously driven by the display multiplexing code, so the built-in `Serial` path is not usable while the display is active.

Board routing confirms that `JP1` exposes `VCC`, `E1`, `E2`, and `GND`. On this board:

- `JP1 pin 1 = VCC`
- `JP1 pin 2 = E1 = PC0`
- `JP1 pin 3 = E2 = PC1`
- `JP1 pin 4 = GND`

The current firmware uses `PC3` and `PC7` for buttons but does not use `PC0` or `PC1`, making `PC0` a safe debug TX candidate.

## Chosen Approach

Implement option 1:

- Add a new PlatformIO environment named `jp1debug`.
- Add a tiny TX-only bit-banged logger module.
- Use `PC0` (`JP1 pin 2`, net `E1`) as the TX pin.
- Keep the logger disabled in all existing environments.

## Design

### Logger module

Create a small `DebugSerial` library with a minimal API:

- `debuglog::begin()`
- `debuglog::write(uint8_t)`
- `debuglog::print(const char *)`
- `debuglog::println(const char *)`
- `debuglog::printlnHex8(uint8_t)`

Implementation details:

- TX-only, idle-high UART framing.
- Default baud rate: `9600`.
- Use direct port access on `PC0`.
- Disable interrupts while transmitting each byte to keep timing stable enough for sparse debug output.

### Firmware integration

Initialize the logger from normal startup code only when the debug env enables it. Add a few initial logs so the env is immediately useful:

- boot start
- shutdown start
- wakeup complete

Keep logging out of ADC, timer, and display ISRs.

### Build isolation

The new environment adds compile-time flags only for the debug build. Existing envs such as `attiny88`, `rxdiag`, `rxstore`, `debug`, and `diag` keep their current behavior.

## Constraints and Risks

- Because interrupts are masked during each transmitted byte, heavy logging will pause display multiplexing briefly and may cause visible flicker.
- This is suitable for sparse debug messages, not a high-volume console.
- RX is intentionally not supported in this first version.

## Verification

Given the embedded setup, verification will use build-based checks:

1. Confirm `pio run -e jp1debug` initially fails before the env exists.
2. Add the env and confirm the build then fails for the missing logger API.
3. Implement the logger and integration.
4. Confirm `pio run -e jp1debug` succeeds.

## Wiring

Use a USB-UART adapter as follows:

- adapter `RX` -> `JP1 pin 2` (`PC0`, TX)
- adapter `GND` -> `JP1 pin 4`

Leave adapter `TX` disconnected for this version.
