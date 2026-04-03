# Build Environment Consolidation Design

**Goal:** Replace the historical AVR build-env sprawl with four named builds that match bench usage and make `jp1debug` the actual RX debug firmware.

## Approved Environment Names

- `release`: normal receiver firmware with storage enabled
- `debugwire`: the same receiver firmware with debugWIRE settings
- `jp1debug`: receiver diagnostic build with JP1 serial logging and no storage
- `hwdiag`: button/display hardware diagnostics only

## Removed Environments

The obsolete historical envs are removed entirely:

- `attiny88`
- `debug`
- `diag`
- `rxdiag`
- `rxstore`
- `jp1rxdebug`
- `jp1rxstore`

## Runtime Behavior

`release` and `debugwire` both boot straight into receive mode, sample `PA0`, and store complete frames through the EEPROM storage path.

`jp1debug` also boots straight into receive mode, but keeps storage disabled and emits modem activity and receiver-state logs over `JP1 pin 2` (`PC0`). This makes it the single bench-debug build for verifying that the ADC path and demodulator are active.

`hwdiag` remains independent from the modem stack and is reserved for low-level display/button checks.

## Modem Tuning

`jp1debug` must not keep the broken `MODEM_ACTIVITY_THRESHOLD=3` setting. The diagnostic build uses a sane threshold/bitsize combination so activity can cross the threshold and the demodulator can change state instead of sticking in one bucket.

Normal RX/store builds keep the existing store-oriented settings unless explicitly tuned later.

## Documentation

The README should describe the four remaining builds and explicitly note that the continuous sine-wave helper is only an analog-path test, not a valid modem frame source.
