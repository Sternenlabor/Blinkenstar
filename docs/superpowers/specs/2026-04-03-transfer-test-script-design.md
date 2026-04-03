# Transfer Test Script Design

## Goal

Add a one-shot Node script that emits the same two transfer waveforms the live editor at `https://blinkenstar.sternenlabor.de/` uses, but with a locally generated, easy-to-recognize test pattern.

## Scope

- Recreate the live site's transfer framing and audio encoder logic in local ESM `.mjs` code.
- Generate a distinctive text payload that can be recognized on-device.
- Expose the script as `npm run transfer:test`.
- Keep the implementation self-contained in this repo without depending on a browser session or the live site at runtime.

## Encoding Strategy

The live site currently transmits two back-to-back formats for compatibility:

1. Legacy transfer:
- Start markers: `0xA5 0xA5 0xA5 0x5A`
- Per-pattern marker: `0x0F 0xF0`
- End markers: `0x84 0x84 0x84`
- Legacy shaped waveform and long sync preamble

2. Alternate transfer:
- Start markers: `0x99 0x99`
- Per-pattern marker: `0xA9 0xA9`
- End markers: `0x84 0x84`
- Newer square-wave symbol logic with periodic sync inserts

Both formats apply the same Hamming parity byte after every two payload bytes.

## Test Payload

The local script should transmit a single text animation with a message such as `TEST <TOKEN>`, where `<TOKEN>` is random by default and overridable in tests. A text payload is sufficient to validate end-to-end transfer because it exercises the same framing, parity, and byte transport path as the editor.

## Files

- Create `scripts/lib/transfer-tone.mjs` for transfer payload building, byte framing, parity generation, waveform generation, and PCM conversion.
- Create `scripts/play-transfer-once.mjs` for one-shot playback.
- Add `test/transfer-tone.test.mjs` for framing and script registration checks.
- Update `package.json` to expose `transfer:test`.
- Update `README.md` with transfer-test usage.

## Verification

- Unit tests must confirm the generated raw framed bytes contain the expected start/end markers and identifiable token bytes.
- `npm run transfer:test` should play once and exit.
- Hardware validation should use `jp1debug` to confirm the transfer produces receiver-state activity beyond a plain tone.
