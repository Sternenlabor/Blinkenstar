# Bench And Debug

## Audio Bench Scripts

The repo ships two Node-based bench entry points:

- `npm run tone:test`
  Plays a continuous `1 kHz` sine wave through the default system output.
- `npm run transfer:test`
  Plays a one-shot transfer payload that mirrors the upload framing used by the project.

Install the dependency first:

```bash
npm install
```

Run the scripts from the repo root:

```bash
npm run tone:test
npm run transfer:test
```

## What Each Script Is For

`tone:test` is for:

- checking that the analog path into the board is alive
- verifying basic audio routing
- isolating pure audio problems from framing/protocol problems

`transfer:test` is for:

- exercising the real receive path
- testing framed transfer decode
- testing storage writes and on-device playback
- confirming that stalled transfers fall back to the on-device transmission-error path instead of leaving the parser half-open

The tone script does not produce modem framing markers by itself, so it should not be expected to store content.

## JP1 Debug Logging

Use the `jp1debug` environment when you need receive-side serial diagnostics.

Build:

```bash
cd firmware
pio run -e jp1debug
```

JP1 wiring:

- `JP1 pin 2` is TX on `PC0`
- `JP1 pin 4` is GND
- connect USB-UART `RX` to `JP1 pin 2`
- connect USB-UART `GND` to `JP1 pin 4`

Notes:

- `jp1debug` is intentionally SRAM-constrained
- it suppresses the normal boot message
- it disables storage and uses a debug-oriented receive path

## Hardware Bring-Up Images

Use `hwdiag` for:

- button wiring checks
- matrix wiring checks
- comparing display startup against the full runtime

Use `release` for:

- actual receive/store behavior
- normal user-visible text and animation playback

Use `debugwire` only if you are actively working on debugWIRE support and willing to reduce flash usage first.

## Typical Bring-Up Sequence

1. `npm test`
2. `cd firmware && pio run -e hwdiag`
3. verify button and matrix wiring on-device
4. `pio run -e release`
5. `npm run tone:test` to confirm the analog path
6. `npm run transfer:test` to validate framed receive and storage
7. switch to `jp1debug` only if runtime logging is needed

If a transfer starts but then goes silent, the firmware now times out after roughly four seconds, clears the partial receive state, and shows the built-in transmission error message.
