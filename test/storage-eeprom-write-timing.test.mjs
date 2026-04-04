import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const storageSourcePath = path.join(repoRoot, 'firmware', 'lib', 'Storage', 'Storage.cpp')

test('storage keeps the original EEPROM retry window and busy-wait pacing', () => {
    const storageSource = fs.readFileSync(storageSourcePath, 'utf8')

    assert.match(storageSource, /for \(uint8_t num_tries = 0; num_tries < 32; num_tries\+\+\)/)
    assert.match(storageSource, /if \(num_tries > 0\)\s*\{\s*_delay_us\(500\);\s*\}/)
    assert.match(storageSource, /for \(uint8_t num_tries = 0; num_tries < 32; num_tries\+\+\)/)
})
