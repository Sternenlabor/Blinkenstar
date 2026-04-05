import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const systemHeaderPath = path.join(repoRoot, 'firmware', 'lib', 'System', 'System.h')
const systemSourcePath = path.join(repoRoot, 'firmware', 'lib', 'System', 'System.cpp')

/**
 * Verify that `System` keeps explicit state for browsing stored patterns with the front buttons.
 */
test('system tracks the currently selected stored pattern and button browse state', () => {
    const systemHeader = fs.readFileSync(systemHeaderPath, 'utf8')

    assert.match(systemHeader, /uint8_t current_pattern_index_ = 0;/)
    assert.match(systemHeader, /uint8_t button_mask_ = 0;/)
    assert.match(systemHeader, /uint32_t button_debounce_until_ms_ = 0;/)
    assert.doesNotMatch(systemHeader, /button_raw_mask_/)
    assert.doesNotMatch(systemHeader, /button_stable_mask_/)
    assert.doesNotMatch(systemHeader, /button_candidate_since_ms_/)
})

/**
 * Verify that single-button release steps through stored patterns like the upstream firmware.
 */
test('system advances and rewinds stored patterns with wraparound on button release', () => {
    const systemSource = fs.readFileSync(systemSourcePath, 'utf8')

    assert.match(systemSource, /button_mask_ \|= BUTTON_NEXT;/)
    assert.match(systemSource, /button_mask_ \|= BUTTON_PREVIOUS;/)
    assert.doesNotMatch(systemSource, /raw_button_mask != button_raw_mask_/)
    assert.match(systemSource, /if \(\(long\)\(loop_now - button_debounce_until_ms_\) >= 0\)/)
    assert.doesNotMatch(systemSource, /if \(!button1_is_low\(\) && !button2_is_low\(\)\)\s*\{\s*storage\.enable\(\);/s)
    assert.match(systemSource, /if \(button_mask_ == BUTTON_NEXT\)\s*\{\s*\/\*[\s\S]*?storage\.enable\(\);/s)
    assert.match(systemSource, /else if \(button_mask_ == BUTTON_PREVIOUS\)\s*\{\s*\/\/ Mirror the upstream browse flow without continuous idle bus traffic\.\s*storage\.enable\(\);/s)
    assert.match(systemSource, /button_debounce_until_ms_ = loop_now \+ BUTTON_BROWSE_COOLDOWN_MS;/)
    assert.match(systemSource, /current_pattern_index_ = \(current_pattern_index_ \+ 1\) % storage\.numPatterns\(\);/)
    assert.match(systemSource, /if \(current_pattern_index_ == 0\)\s*\{\s*current_pattern_index_ = storage\.numPatterns\(\) - 1;/)
    assert.match(systemSource, /modemReceiver\.showStoredPattern\(current_pattern_index_\)/)
    assert.match(systemSource, /current_pattern_index_ = 0;/)
})
