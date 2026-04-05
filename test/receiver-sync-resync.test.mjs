import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const receiverSourcePath = path.join(repoRoot, 'firmware', 'lib', 'Modem', 'Receiver.cpp')

/**
 * Verify that broken marker bytes force the parser all the way back to START1.
 */
test('receiver fully resynchronizes after invalid marker bytes like the upstream firmware', () => {
    const receiverSource = fs.readFileSync(receiverSourcePath, 'utf8')

    assert.match(
        receiverSource,
        /case NEXT_BLOCK:[\s\S]*?else\s*\{\s*\/\/ Resynchronize fully after a broken post-start marker so the next real frame can reacquire START1\.\s*state_ = START1;[\s\S]*?\}/
    )
    assert.match(
        receiverSource,
        /case PATTERN2:[\s\S]*?else\s*\{\s*\/\/ A broken PATTERN marker means we lost framing, so restart from START1 like the original firmware\.\s*state_ = START1;[\s\S]*?\}/
    )
})
