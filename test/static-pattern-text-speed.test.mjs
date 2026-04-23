import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const staticPatternsPath = path.join(repoRoot, 'firmware', 'lib', 'System', 'static_patterns.h')

/**
 * Verify that the empty-storage text keeps the upstream no-delay scroll speed.
 */
test('empty text pattern uses the upstream no-delay metadata bytes', () => {
    const staticPatterns = fs.readFileSync(staticPatternsPath, 'utf8')

    assert.match(staticPatterns, /0x10, 0x28,\s*0xE0, 0x00,/s)
    assert.match(staticPatterns, /0x10, 0x26,\s*0xE0, 0x00,/s)
    assert.match(
        staticPatterns,
        /const uint8_t PROGMEM timeoutPattern\[\] = \{\s*0x10, 0x15,\s*0xE0, 0x00,/s
    )
})

/**
 * Verify that the empty-storage text keeps the original boot intro content.
 */
test('empty storage message keeps the original boot intro text', () => {
    const staticPatterns = fs.readFileSync(staticPatternsPath, 'utf8')

    assert.match(staticPatterns, /' ', 1, 'v', FW_REV_MAJOR \+ '0', '\.', FW_REV_MINOR \+ '0', ' ', '-'/)
    assert.match(staticPatterns, /'B', 'l', 'i', 'n', 'k', 'e', 'n', 's', 't', 'a', 'r'/)
    assert.match(staticPatterns, /'S', 't', 'o', 'r', 'a', 'g', 'e', ' ', 'i', 's',\s*' ', 'e', 'm', 'p', 't', 'y'/s)
})
