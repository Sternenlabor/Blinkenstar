import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const displaySourcePath = path.join(repoRoot, 'firmware', 'lib', 'Display', 'Display.cpp')
const systemSourcePath = path.join(repoRoot, 'firmware', 'lib', 'System', 'System.cpp')

/**
 * Verify that the display multiplex timer matches the upstream 256 microsecond cadence.
 */
test('display refresh period matches the upstream 256 microsecond multiplex timing', () => {
    const displaySource = fs.readFileSync(displaySourcePath, 'utf8')

    assert.match(displaySource, /timer\.initialize\(256\);/)
})

/**
 * Verify that the timer ISR only flags pending animation work and leaves chunk loads to the main loop.
 */
test('display updates run from the main loop instead of inside the multiplex interrupt', () => {
    const displaySource = fs.readFileSync(displaySourcePath, 'utf8')
    const systemSource = fs.readFileSync(systemSourcePath, 'utf8')

    assert.match(displaySource, /need_update = 1;/)
    assert.doesNotMatch(displaySource, /if \(\+\+update_cnt == update_threshold\)\s*\{\s*update_cnt = 0;\s*need_update = 1;\s*update\(\);/s)
    assert.match(systemSource, /display\.update\(\);/)
})
