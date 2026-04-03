import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const systemPath = path.join(repoRoot, 'firmware', 'lib', 'System', 'System.cpp')
const displayHeaderPath = path.join(repoRoot, 'firmware', 'lib', 'Display', 'Display.h')

test('shutdown wake path preserves the active display state across the temporary power-down animation', () => {
    const systemSource = fs.readFileSync(systemPath, 'utf8')
    const displayHeader = fs.readFileSync(displayHeaderPath, 'utf8')

    assert.match(displayHeader, /struct DisplayState/)
    assert.match(displayHeader, /void snapshotState\(DisplayState &state\) const;/)
    assert.match(displayHeader, /void restoreState\(const DisplayState &state\);/)

    const snapshotCall = 'display.snapshotState(preShutdownDisplayState);'
    const showCall = 'playShutdownAnimation();'
    const enableCall = 'display.enable();'
    const restoreCall = 'display.restoreState(preShutdownDisplayState);'

    const snapshotIndex = systemSource.indexOf(snapshotCall)
    const showIndex = systemSource.indexOf(showCall)
    const enableIndex = systemSource.indexOf(enableCall)
    const restoreIndex = systemSource.indexOf(restoreCall)

    assert.notEqual(snapshotIndex, -1, 'expected shutdown() to snapshot the active display state')
    assert.notEqual(showIndex, -1, 'expected shutdown animation to be shown')
    assert.ok(snapshotIndex < showIndex, 'expected display state snapshot before shutdown animation starts')

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
