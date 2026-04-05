import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const displayHeaderPath = path.join(repoRoot, 'firmware', 'lib', 'Display', 'Display.h')
const displaySourcePath = path.join(repoRoot, 'firmware', 'lib', 'Display', 'Display.cpp')
const receiverSourcePath = path.join(repoRoot, 'firmware', 'lib', 'Modem', 'Receiver.cpp')

/**
 * Verify that large storage-backed animations are not truncated to one 128-byte payload chunk.
 */
test('storage-backed animations preserve their full length and reload later EEPROM chunks', () => {
    const displayHeader = fs.readFileSync(displayHeaderPath, 'utf8')
    const displaySource = fs.readFileSync(displaySourcePath, 'utf8')
    const receiverSource = fs.readFileSync(receiverSourcePath, 'utf8')

    assert.doesNotMatch(receiverSource, /if \(anim\.length > 128\)\s*\{\s*anim\.length = 128;\s*\}/)
    assert.match(displayHeader, /void showFromStorage\(const animation_t \*anim\);/)
    assert.match(displaySource, /void Display::showFromStorage\(const animation_t \*anim\)\s*\{\s*startAnimation_\(anim, true\);/s)
    assert.match(displaySource, /if \(current_anim_storage_backed && current_anim->length > 128\)/)
    assert.match(displaySource, /storage\.loadChunk\(str_chunk, current_anim->data\);/)
    assert.match(receiverSource, /storage\.load\(idx, display_payload_buf\);\s*return showPayloadBuffer\(display_payload_buf, true\);/s)
})
