import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const receiverSourcePath = path.join(repoRoot, 'firmware', 'lib', 'Modem', 'Receiver.cpp')

/**
 * Verify that a valid transfer start switches the display to the upstream flashing animation.
 */
test('receiver shows the upstream flashing pattern while a frame is being received', () => {
    const receiverSource = fs.readFileSync(receiverSourcePath, 'utf8')

    assert.match(receiverSource, /static uint8_t flashing_pattern_buf\[sizeof\(flashingPattern\)\];/)
    assert.match(receiverSource, /static void showTransferFlashPattern\(\)/)
    assert.match(receiverSource, /for \(uint8_t i = 0; i < sizeof\(flashingPattern\); \+\+i\)\s*\{\s*flashing_pattern_buf\[i\] = pgm_read_byte\(flashingPattern \+ i\);/s)
    assert.match(receiverSource, /showPayloadBuffer\(flashing_pattern_buf\);/)
    assert.match(receiverSource, /storage\.reset\(\);[\s\S]*?showTransferFlashPattern\(\);/s)
})
