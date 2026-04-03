# Transfer Test Script Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a one-shot local transfer test script that reproduces the live site's audio upload framing with a distinctive text payload.

**Architecture:** Implement a small ESM `.mjs` encoder module that builds both compatibility transfer formats from one text pattern, converts the generated float samples into 16-bit PCM, and feeds them to `speaker` from a simple CLI script. Keep tests focused on frame bytes and npm script registration.

**Tech Stack:** Node.js ESM (`.mjs`), `speaker`, `node:test`

---

### Task 1: Lock Transfer Framing in Tests

**Files:**
- Create: `test/transfer-tone.test.mjs`
- Modify: `/Users/afiedler/Documents/privat/SL/Blinkenstar/package.json`

- [ ] **Step 1: Write the failing test**

Create tests that import `scripts/lib/transfer-tone.mjs`, assert legacy and alternate raw frame markers, assert the chosen token bytes are embedded, and assert `package.json` exposes `transfer:test`.

- [ ] **Step 2: Run test to verify it fails**

Run: `node --test test/transfer-tone.test.mjs`
Expected: FAIL because the transfer encoder module and script entry do not exist yet.

- [ ] **Step 3: Implement minimal script registration**

Add `transfer:test` to `package.json` pointing at `node scripts/play-transfer-once.mjs`.

- [ ] **Step 4: Re-run test**

Run: `node --test test/transfer-tone.test.mjs`
Expected: still FAIL because encoder functions are not implemented yet.

### Task 2: Implement the Transfer Encoder

**Files:**
- Create: `scripts/lib/transfer-tone.mjs`
- Test: `test/transfer-tone.test.mjs`

- [ ] **Step 1: Write the minimal encoder API**

Export helpers for:
- building a default identifiable text pattern
- generating raw framed bytes for legacy and alternate modes
- generating combined float samples
- converting samples into a PCM buffer

- [ ] **Step 2: Implement byte framing and parity**

Mirror the site's framing:
- legacy start/end markers and `0x0F 0xF0` block markers
- alternate start/end markers and `0xA9 0xA9` block markers
- Hamming parity byte after each byte pair

- [ ] **Step 3: Implement waveform generation**

Mirror the live site's legacy and alternate symbol and sync waveforms closely enough to reproduce the same transfer logic.

- [ ] **Step 4: Run focused tests**

Run: `node --test test/transfer-tone.test.mjs`
Expected: PASS

### Task 3: Add the One-Shot Playback CLI

**Files:**
- Create: `scripts/play-transfer-once.mjs`
- Modify: `README.md`

- [ ] **Step 1: Implement one-shot playback**

Build a random token, log the text payload, generate the PCM buffer, play it once through `speaker`, then exit.

- [ ] **Step 2: Document usage**

Add a short README section describing `npm run transfer:test` and that it emits the same transfer logic as the live editor.

- [ ] **Step 3: Run full verification**

Run:
- `node --test`
- `npm run transfer:test`

Expected:
- tests pass
- the script starts audio playback once and exits cleanly

### Task 4: Bench Validation

**Files:**
- None

- [ ] **Step 1: Flash or keep `jp1debug` on the device**

Use the existing `jp1debug` firmware so JP1 logs remain available during transfer playback.

- [ ] **Step 2: Run a live transfer capture**

Run `npm run transfer:test` while capturing JP1 serial.
Expected: receiver-state logs move beyond plain tone detection and ideally show `START`/`PATTERN` markers.
