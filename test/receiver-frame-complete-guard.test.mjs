import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const receiverHeaderPath = path.join(repoRoot, 'firmware', 'lib', 'Modem', 'Receiver.h')
const receiverSourcePath = path.join(repoRoot, 'firmware', 'lib', 'Modem', 'Receiver.cpp')

/**
 * Verify that the receiver refuses to finalize a frame until at least one payload block completed.
 */
test('receiver only finalizes a frame after at least one payload block has completed', () => {
    const receiverHeader = fs.readFileSync(receiverHeaderPath, 'utf8')
    const receiverSource = fs.readFileSync(receiverSourcePath, 'utf8')

    assert.match(receiverHeader, /bool frame_payload_complete_ = false;/)
    assert.match(receiverSource, /frame_payload_complete_ = false;/)
    assert.match(receiverSource, /frame_payload_complete_ = true;/)
    assert.match(
        receiverSource,
        /else if \(b == BYTE_END\)\s*\{\s*if \(!frame_payload_complete_\)\s*\{\s*state_ = START1;[\s\S]*?fecModem\.clear\(\);\s*break;\s*\}/
    )
})

/**
 * Verify that completed payload pages are committed before the trailing END marker is required.
 */
test('receiver persists completed payload pages before waiting for END markers', () => {
    const receiverSource = fs.readFileSync(receiverSourcePath, 'utf8')

    assert.match(
        receiverSource,
        /case DATA_FIRSTBLOCK:[\s\S]*?if \(remaining_ == 0\)[\s\S]*?storage\.save\(rx_buf_\);\s*storage\.sync\(\);/
    )
    assert.match(receiverSource, /case DATA:[\s\S]*?if \(remaining_ == 0\)[\s\S]*?storage\.append\(rx_buf_\);\s*storage\.sync\(\);/)
})
