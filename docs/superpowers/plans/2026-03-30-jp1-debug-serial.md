# JP1 Debug Serial Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a `jp1debug` firmware env that emits sparse TX-only serial logs on `JP1 pin 2` (`PC0`).

**Architecture:** Introduce a tiny `DebugSerial` module that bit-bangs TX on `PC0` only when a new debug env enables it. Integrate the logger from foreground firmware code so normal builds and ISR timing stay unchanged unless the debug env is selected.

**Tech Stack:** PlatformIO, ATTinyCore/Arduino for AVR, direct AVR register access

---

### Task 1: Add the new PlatformIO env

**Files:**
- Modify: `firmware/platformio.ini`

- [ ] **Step 1: Write the failing check**

Run: `~/.platformio/penv/bin/pio run -e jp1debug`

Expected: fail because environment `jp1debug` does not exist yet.

- [ ] **Step 2: Add the new env**

Create a `jp1debug` env extending `env:attiny88` with flags enabling the logger on `PC0` at `9600` baud.

- [ ] **Step 3: Run the build again**

Run: `~/.platformio/penv/bin/pio run -e jp1debug`

Expected: fail next for missing debug logger implementation or references.

### Task 2: Add the logger module

**Files:**
- Create: `firmware/lib/DebugSerial/DebugSerial.h`
- Create: `firmware/lib/DebugSerial/DebugSerial.cpp`
- Create: `firmware/test/DebugSerialCompile.cpp`

- [ ] **Step 1: Write the failing compile smoke test**

Create a small compile smoke file that includes `DebugSerial.h` and uses the public API.

- [ ] **Step 2: Run compile smoke to verify it fails**

Run: `~/.platformio/packages/toolchain-atmelavr/bin/avr-g++ -x c++ -std=gnu++11 -DF_CPU=8000000L -I firmware/lib/DebugSerial -I ~/.platformio/packages/framework-arduino-avr-attiny -I ~/.platformio/packages/framework-arduino-avr-attiny/avr/cores/tiny -mmcu=attiny88 -c firmware/test/DebugSerialCompile.cpp -o /tmp/DebugSerialCompile.o`

Expected: fail because `DebugSerial.h` does not exist yet.

- [ ] **Step 3: Implement the minimal logger**

Add a TX-only bit-banged logger on `PC0` with the approved API.

- [ ] **Step 4: Re-run compile smoke**

Expected: pass.

### Task 3: Integrate the logger into startup/shutdown flow

**Files:**
- Modify: `firmware/lib/System/System.cpp`

- [ ] **Step 1: Add a small set of debug messages**

Log boot, shutdown, and wakeup events from foreground code only.

- [ ] **Step 2: Build the new env**

Run: `~/.platformio/penv/bin/pio run -e jp1debug`

Expected: pass.

### Task 4: Verify and document usage

**Files:**
- Modify: `README.md` if needed

- [ ] **Step 1: Re-run the full proof build**

Run: `~/.platformio/penv/bin/pio run -e jp1debug`

Expected: pass with exit code 0.

- [ ] **Step 2: Summarize the wiring and usage**

Document `JP1 pin 2 = TX` and `JP1 pin 4 = GND`, with adapter `RX` wired to JP1 TX.
