import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const systemPath = path.join(repoRoot, 'firmware', 'lib', 'System', 'System.cpp')
const displayHeaderPath = path.join(repoRoot, 'firmware', 'lib', 'Display', 'Display.h')
const staticPatternsPath = path.join(repoRoot, 'firmware', 'lib', 'System', 'static_patterns.h')

test('shutdown wake path preserves the active display state across the temporary power-down animation', () => {
    const systemSource = fs.readFileSync(systemPath, 'utf8')
    const displayHeader = fs.readFileSync(displayHeaderPath, 'utf8')

    assert.match(displayHeader, /struct DisplayState/)
    assert.match(displayHeader, /void snapshotState\(DisplayState &state\) const;/)
    assert.match(displayHeader, /void restoreState\(const DisplayState &state\);/)

    const snapshotCall = 'display.snapshotState(preShutdownDisplayState);'
    const freezeCall = 'display.freezeState(preShutdownDisplayState);'
    const showCall = 'playShutdownAnimation();'
    const enableCall = 'display.enable();'
    const restoreCall = 'display.restoreState(preShutdownDisplayState);'

    const snapshotIndex = systemSource.indexOf(snapshotCall)
    const freezeIndex = systemSource.indexOf(freezeCall)
    const showIndex = systemSource.indexOf(showCall)
    const enableIndex = systemSource.indexOf(enableCall)
    const restoreIndex = systemSource.indexOf(restoreCall)

    assert.notEqual(snapshotIndex, -1, 'expected shutdown() to snapshot the active display state')
    assert.notEqual(freezeIndex, -1, 'expected shutdown() to freeze the saved frame before the manual shutdown animation')
    assert.notEqual(showIndex, -1, 'expected shutdown animation to be shown')
    assert.ok(snapshotIndex < freezeIndex, 'expected display state snapshot before freezing the display')
    assert.ok(freezeIndex < showIndex, 'expected frozen saved frame before shutdown animation starts')

    assert.notEqual(enableIndex, -1, 'expected wake path to re-enable the display')
    assert.notEqual(restoreIndex, -1, 'expected wake path to restore the pre-shutdown display state')
    assert.ok(enableIndex < restoreIndex, 'expected display state restore after display hardware is re-enabled')
})

test('shutdown animation is streamed from PROGMEM instead of reserving a 64-byte SRAM buffer', () => {
    const systemSource = fs.readFileSync(systemPath, 'utf8')

    assert.doesNotMatch(systemSource, /static uint8_t animationBuffer\[64\];/)
    assert.match(systemSource, /playShutdownAnimation\(\);/)
    assert.match(systemSource, /pgm_read_byte\(shutdownPattern \+ 4 \+ data_offset \+ col\)/)
})

test('shutdown animation timing matches the upstream cadence independently of the boot text speed', () => {
    const systemSource = fs.readFileSync(systemPath, 'utf8')
    const staticPatterns = fs.readFileSync(staticPatternsPath, 'utf8')

    assert.match(systemSource, /const uint32_t full_refresh_us = 8UL \* 256UL;/)
    assert.match(staticPatterns, /const uint8_t PROGMEM shutdownPattern\[\] = \{\s*0x20, 0x40,\s*0x0E, 0x0F,/s)
})
