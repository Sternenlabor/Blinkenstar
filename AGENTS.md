# AGENTS

## Project Overview

- Repository: `Blinkenstar`
- Purpose: hardware, firmware, and bench-test utilities for the Blinkenstar board.
- Hardware design sources live in `hardware/Blinkenstar/`, including the KiCad schematic, PCB, BOM exports, fabrication assets, and local footprint/symbol libraries.
- Firmware source lives in `firmware/src/` and `firmware/lib/`.
- Maintained firmware helper scripts live in `firmware/scripts/`.
- Distributable firmware artifacts copied out of PlatformIO builds live in `firmware/dist/`.
- PlatformIO configuration lives in `firmware/platformio.ini`.
- Node-based audio helpers and bench scripts live in `scripts/` and `scripts/lib/`.
- Automated host-side tests live in `test/`.
- Firmware host probes and compile checks live in `firmware/test/`.
- Design notes and scoped implementation records live in `docs/superpowers/specs/` and `docs/superpowers/plans/`.

## Build, Run, Test

- Install Node dependencies: `npm install`
- Run the consolidated JavaScript and host-side test suite: `npm test`
- Play the continuous sine-wave bench tone: `npm run tone:test`
- Play the one-shot transfer payload bench test: `npm run transfer:test`
- Build firmware from `firmware/` with the checked-in PlatformIO environments, for example `pio run -e release`.
- Use `firmware/scripts/flash.sh` when you need a maintained build-copy-flash workflow with fuse programming.
- The supported firmware environments are `release`, `debugwire`, `jp1debug`, and `hwdiag`.
- Prefer repo scripts and the checked-in PlatformIO environments over ad hoc commands when an equivalent workflow already exists.
- If you touch `scripts/`, `test/`, or `package.json` scripts, run `npm test`.
- If you touch transfer framing or transfer-tone generation, run `npm test` and use `npm run transfer:test` when bench validation matters.
- If you touch sine-wave generation or audio-output helpers, run `npm test` and use `npm run tone:test` when output-path validation matters.
- If you touch `firmware/` source or `firmware/platformio.ini`, run `npm test` and build the relevant PlatformIO environment or environments.

## Coding Style And Naming Conventions

- Follow `.prettierrc.json`: 4-space indent, single quotes, no semicolons, no trailing commas, `printWidth` 140.
- Keep Node-side code in ESM `.mjs` and use explicit relative file extensions in imports.
- Keep reusable JavaScript generators and encoders in `scripts/lib/` and keep `scripts/` entry points thin.
- Preserve existing repo naming patterns. JavaScript files use lowercase kebab-case `.mjs`, while firmware modules use PascalCase header/source pairs.
- Add JSDoc comments for every function or method in first-party `*.js` and `*.mjs` files under source, library, and test directories, including private helpers and test callbacks.
- Add Doxygen-style comments for every function or method in first-party C/C++ source, library, and test files, including private helpers, static functions, and ISR entry points.
- Add brief comments where behavior is not obvious, especially around modem thresholds, FEC or Hamming logic, timing assumptions, display multiplexing, pin mappings, and debug wiring.
- Add inline comments wherever a decision, hardware assumption, or protocol behavior would otherwise be unclear to the next reader.
- Prefer small focused helpers over large mixed-responsibility files.
- Do not casually rename or repurpose the consolidated PlatformIO environments. The test suite asserts their names and intended roles.

## Testing Guidelines

- Use `npm test` as the default verification gate.
- Add or update tests in `test/` for every JavaScript behavior change, including package scripts and configuration regressions.
- When firmware behavior changes, add or update a host-testable probe in `firmware/test/` when practical, then build the relevant PlatformIO environment.
- Keep tests focused on observable behavior such as framing bytes, PCM/sample bounds, environment names, compile success, and documented runtime roles.
- Do not special-case production logic for a single test token, serial trace, board revision label, or fixture name just to make one test pass.
- Keep diagnostic and transfer behavior generic. Derive behavior from protocol structure, thresholds, and state transitions rather than hardcoded sample-specific data.

## Hardware Artifact And Sample Data Rules

- Treat `hardware/Blinkenstar/Blinkenstar.kicad_sch` and `hardware/Blinkenstar/Blinkenstar.kicad_pcb` as the hardware source of truth.
- Treat `hardware/Blinkenstar/gerber/`, `hardware/Blinkenstar/production/`, BOM exports, PDFs, and backup ZIPs as generated artifacts or snapshots. Do not hand-edit them.
- Regenerate fabrication outputs from the KiCad sources when hardware changes need release-ready artifacts.
- Do not add proprietary customer files, private manufacturing data, or identifying bench captures to the repository.
- Prefer generic synthetic transfer tokens, public component references, and repo-owned assets in tests and documentation.

## Commit And Pull Request Guidelines

- Commit messages should start with a conventional prefix such as `fix:`, `feat:`, `chore:`, `docs:`, or `test:` followed by a short imperative summary.
- Keep firmware, hardware-source, generated-artifact, and Node utility changes separated when practical so reviews stay readable.
- When hardware sources change, state whether fabrication outputs, BOM exports, or production ZIPs were regenerated.
- Keep merge or pull request summaries concise and include the verification that was run.
- Attach photos, serial logs, or scope captures for hardware-facing or diagnostic changes when they materially clarify behavior.

## Documentation Guidelines

- Keep the root `README.md` as the main entry point for the project.
- Keep scoped implementation plans in `docs/superpowers/plans/` and design notes in `docs/superpowers/specs/`.
- Update documentation alongside changes to PlatformIO environments, bench procedures, transfer framing, audio tooling, or hardware release artifacts.
- Document new toolchain prerequisites explicitly instead of hiding them in one-off commands or comments.

## Security And Configuration Tips

- Keep secrets, local serial-port names, programmer-specific paths, and machine-local overrides out of Git.
- Prefer local-first and offline-friendly workflows. Document any new outbound network behavior before adding it.
- Be deliberate with generated binaries and backup archives. Only commit artifacts the repository intentionally tracks.
- Validate externally supplied or user-provided data before feeding it into audio generation, transfer encoding, or firmware-side decode paths.
