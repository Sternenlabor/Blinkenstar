import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const systemSourcePath = path.join(repoRoot, 'firmware', 'lib', 'System', 'System.cpp')

/**
 * Verify that cold boot can clear persisted storage before the normal restore path runs.
 */
test('system factory reset clears storage when both buttons are held during power-on', () => {
    const systemSource = fs.readFileSync(systemSourcePath, 'utf8')

    assert.match(systemSource, /static bool factoryResetRequested\(\)/)
    assert.match(systemSource, /delay\(25\);/)
    assert.match(systemSource, /return button1_is_low\(\) && button2_is_low\(\);/)
    assert.match(systemSource, /static bool resetStorageIfRequested\(\)/)
    assert.match(systemSource, /storage\.enable\(\);\s*storage\.reset\(\);\s*storage\.sync\(\);\s*return true;/s)
    assert.match(systemSource, /const bool factory_reset_requested = resetStorageIfRequested\(\);/)
    assert.match(systemSource, /if \(!factory_reset_requested\)\s*\{\s*current_pattern_index_ = 0;\s*modemReceiver\.showStoredPattern\(current_pattern_index_\);/s)
})
