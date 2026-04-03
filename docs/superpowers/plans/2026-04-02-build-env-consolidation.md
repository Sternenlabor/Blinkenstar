# Build Environment Consolidation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Consolidate the AVR firmware environments into four named builds and make `jp1debug` the receiver debug build.

**Architecture:** Treat the change as a configuration regression fix. Lock the intended env matrix with a test first, then rename and collapse the PlatformIO envs, then update the user-facing docs to match the new behavior.

**Tech Stack:** PlatformIO, Node.js built-in test runner, AVR/Arduino firmware

---

### Task 1: Lock the environment matrix with a regression test

**Files:**
- Test: `test/platformio-envs.test.mjs`

- [ ] **Step 1: Write the failing test**

Add a Node test that asserts:
- only `release`, `debugwire`, `jp1debug`, and `hwdiag` exist
- obsolete envs are removed
- `jp1debug` contains modem RX and JP1 serial flags

- [ ] **Step 2: Run test to verify it fails**

Run: `node --test test/platformio-envs.test.mjs`
Expected: FAIL because the old env names still exist and `jp1debug` does not enable the modem.

### Task 2: Consolidate PlatformIO environments

**Files:**
- Modify: `firmware/platformio.ini`

- [ ] **Step 1: Rename the remaining envs**

Replace:
- `attiny88` -> `release`
- `debug` -> `debugwire`
- `diag` -> `hwdiag`

- [ ] **Step 2: Remove obsolete RX envs**

Delete:
- `rxdiag`
- `rxstore`
- `jp1rxdebug`
- `jp1rxstore`

- [ ] **Step 3: Make `jp1debug` the RX debug build**

Ensure `jp1debug` includes:
- `-DENABLE_MODEM`
- `-DRX_NO_STORAGE`
- `-DRX_ALWAYS_ON`
- `-DRX_SLOW_ADC`
- sane modem threshold flags
- `-DJP1_DEBUG_SERIAL`

### Task 3: Update the docs

**Files:**
- Modify: `README.md`
- Create: `docs/superpowers/specs/2026-04-02-build-env-consolidation-design.md`
- Create: `docs/superpowers/plans/2026-04-02-build-env-consolidation.md`

- [ ] **Step 1: Document the four remaining builds**

Explain the purpose of `release`, `debugwire`, `jp1debug`, and `hwdiag`.

- [ ] **Step 2: Clarify the tone generator limitation**

State that the sine-wave helper verifies the analog path only and does not generate modem frames.

### Task 4: Verify the consolidated build matrix

**Files:**
- Test: `test/platformio-envs.test.mjs`
- Verify: `firmware/platformio.ini`

- [ ] **Step 1: Run the Node tests**

Run: `node --test`
Expected: PASS

- [ ] **Step 2: Build each remaining firmware env**

Run:
- `~/.platformio/penv/bin/pio run -e release`
- `~/.platformio/penv/bin/pio run -e debugwire`
- `~/.platformio/penv/bin/pio run -e hwdiag`
- `~/.platformio/penv/bin/pio run -e jp1debug`

Expected: all four builds succeed
