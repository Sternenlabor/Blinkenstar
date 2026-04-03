import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const displayHeaderPath = path.join(repoRoot, 'firmware', 'lib', 'Display', 'Display.h')
const displaySourcePath = path.join(repoRoot, 'firmware', 'lib', 'Display', 'Display.cpp')

test('boot message path streams animation data from PROGMEM instead of copying a RAM buffer', () => {
    const displayHeader = fs.readFileSync(displayHeaderPath, 'utf8')
    const displaySource = fs.readFileSync(displaySourcePath, 'utf8')

    assert.match(displayHeader, /void showBootMessage\(\);/)
    assert.match(displayHeader, /bool current_anim_progmem = false;/)
    assert.doesNotMatch(displaySource, /activeAnimationData\[64\]/)
    assert.doesNotMatch(displaySource, /static animation_t activeAnimation;/)
    assert.match(displaySource, /showBootMessage\(\);/)
    assert.match(displaySource, /current_anim == nullptr && current_anim_progmem/)
    assert.match(displaySource, /pgm_read_byte\(emptyPattern \+ 4 \+ str_pos\)/)
})

test('boot message renderer can be enabled explicitly inside hardware diagnostics', () => {
    const displaySource = fs.readFileSync(displaySourcePath, 'utf8')

    assert.match(displaySource, /#if \(!defined\(DIAG_BUTTONS\) \|\| defined\(DIAG_BOOT_MESSAGE\)\) && !defined\(NO_BOOT_MESSAGE\)/)
})
