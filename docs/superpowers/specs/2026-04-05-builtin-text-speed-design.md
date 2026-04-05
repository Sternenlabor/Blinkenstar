# Built-In Text Speed Design

## Goal

Speed up the built-in `Storage is empty` and `Transmission error` text messages in the `release` firmware without changing the global display refresh cadence.

## Decision

Apply the change only in [`firmware/lib/System/static_patterns.h`](../../../firmware/lib/System/static_patterns.h) by updating the text-pattern metadata bytes.

## Rationale

- The display multiplex rate already matches the upstream `256 us` cadence.
- The visible slowdown comes from the pattern metadata, not from the display engine.
- A metadata-only change avoids touching the display runtime and keeps risk low.

## Scope

- Set `emptyPattern` and `timeoutPattern` to the same faster no-delay scroll speed.
- Add a regression test that locks the selected metadata bytes.
- Verify with `npm test` and `pio run -e release`.
