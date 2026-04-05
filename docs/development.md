# Development Guide

## Default Verification

The default verification gate is:

```bash
npm test
```

That suite covers:

- Node-side audio helpers
- transfer encoder behavior
- structural firmware regressions
- PlatformIO environment contract checks

## Firmware Verification

When firmware code changes, also build the relevant environment:

```bash
cd firmware
pio run -e release
```

Other common builds:

```bash
pio run -e jp1debug
pio run -e hwdiag
```

## Current Build Status Notes

- `release` is the normal firmware verification target
- `jp1debug` is the main receive-side debug target
- `hwdiag` is the low-risk hardware isolation target
- `debugwire` currently exists but does not fit within flash in the checked-in tree

## Source Layout

- [`firmware/src/main.cpp`](../firmware/src/main.cpp)
  Firmware entry point.
- [`firmware/lib/`](../firmware/lib/)
  Private firmware modules.
- [`scripts/`](../scripts/)
  Thin CLI entry points.
- [`scripts/lib/`](../scripts/lib/)
  Reusable host-side helpers.
- [`test/`](../test/)
  Host-side tests and structural regressions.
- [`firmware/test/`](../firmware/test/)
  Firmware-oriented host probes and compile checks.

## Documentation Layout

- [`README.md`](../README.md)
  Main repo entry point.
- [`docs/`](./)
  User-facing and maintainer-facing project documentation.
- [`docs/superpowers/specs/`](./superpowers/specs/)
  Design notes.
- [`docs/superpowers/plans/`](./superpowers/plans/)
  Implementation records.

## Working Style

The repo favors:

- small focused firmware modules
- checked-in named PlatformIO environments instead of ad hoc flags
- host-side structural tests for firmware behavior when direct hardware tests are impractical
- comments around timing, multiplexing, FEC, thresholds, and storage semantics
