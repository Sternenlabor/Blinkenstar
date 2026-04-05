import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const modemSourcePath = path.join(repoRoot, 'firmware', 'lib', 'Modem', 'Modem.cpp')

/**
 * Verify that the demodulator still follows the original fixed-threshold classification path.
 */
test('modem bit classification follows the legacy threshold demodulator from the original firmware', () => {
    const modemSource = fs.readFileSync(modemSourcePath, 'utf8')

    assert.match(
        modemSource,
        /if \(\(activity < ACTIVITY_THRESHOLD\) && \(bitlen_ > \(BITLEN_THRESHOLD << 2\)\)\)\s*\{\s*prev_freq_ = FREQ_NONE;\s*bitcount_ = 0;\s*byte_ = 0;\s*clear\(\);\s*return;\s*\}/
    )
    assert.match(modemSource, /freq_ = \(activity >= ACTIVITY_THRESHOLD\) \? FREQ_HIGH : FREQ_LOW;/)
})
