import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const systemPath = path.join(repoRoot, 'firmware', 'lib', 'System', 'System.cpp')
const receiverHeaderPath = path.join(repoRoot, 'firmware', 'lib', 'Modem', 'Receiver.h')
const receiverSourcePath = path.join(repoRoot, 'firmware', 'lib', 'Modem', 'Receiver.cpp')

/**
 * Verify that boot restores the first stored animation instead of leaving the screen blank.
 */
test('startup restores the first stored pattern instead of leaving the display dark', () => {
    const systemSource = fs.readFileSync(systemPath, 'utf8')
    const receiverHeader = fs.readFileSync(receiverHeaderPath, 'utf8')
    const receiverSource = fs.readFileSync(receiverSourcePath, 'utf8')

    assert.match(receiverHeader, /bool showStoredPattern\(uint8_t idx = 0\);/)
    assert.match(receiverSource, /static uint8_t display_payload_buf\[132\];/)
    assert.match(receiverSource, /bool ModemReceiver::showStoredPattern\(uint8_t idx\)/)
    assert.match(receiverSource, /storage\.load\(idx, display_payload_buf\);/)
    assert.match(receiverSource, /display\.show\(&anim\);/)
    assert.match(systemSource, /current_pattern_index_ = 0;/)
    assert.match(systemSource, /modemReceiver\.showStoredPattern\(current_pattern_index_\)/)
})
