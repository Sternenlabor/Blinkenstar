import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const twiBusSourcePath = path.join(repoRoot, 'firmware', 'lib', 'TwiBus', 'TwiBus.cpp')

/**
 * Verify that the generic TWI bus helper still keeps the upstream retry count and backoff delay.
 */
test('twi bus keeps the original EEPROM retry window and busy-wait pacing', () => {
    const twiBusSource = fs.readFileSync(twiBusSourcePath, 'utf8')

    assert.match(twiBusSource, /for \(uint8_t num_tries = 0; num_tries < 32; num_tries\+\+\)/)
    assert.match(twiBusSource, /if \(num_tries > 0\)\s*\{\s*_delay_us\(500\);\s*\}/)
    assert.match(twiBusSource, /for \(uint8_t num_tries = 0; num_tries < 32; num_tries\+\+\)/)
})
