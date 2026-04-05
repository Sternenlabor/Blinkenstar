import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const staticPatternsPath = path.join(repoRoot, 'firmware', 'lib', 'System', 'static_patterns.h')

/**
 * Verify that the built-in English text patterns use the faster upstream-style scroll speed.
 */
test('empty and timeout text patterns share the fast no-delay metadata bytes', () => {
    const staticPatterns = fs.readFileSync(staticPatternsPath, 'utf8')

    assert.match(
        staticPatterns,
        /const uint8_t PROGMEM emptyPattern\[\] = \{\s*0x10, 0x26,\s*0xE0, 0x00,/s
    )
    assert.match(
        staticPatterns,
        /const uint8_t PROGMEM timeoutPattern\[\] = \{\s*0x10, 0x15,\s*0xE0, 0x00,/s
    )
})
