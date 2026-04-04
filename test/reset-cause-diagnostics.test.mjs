import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const systemPath = path.join(repoRoot, 'firmware', 'lib', 'System', 'System.cpp')

test('system initialization captures and clears MCUSR before logging or branching on reset cause', () => {
    const systemSource = fs.readFileSync(systemPath, 'utf8')

    assert.match(systemSource, /const uint8_t reset_cause = MCUSR;/)
    assert.match(systemSource, /MCUSR = 0;/)
    assert.match(systemSource, /debuglog::printlnHex8\(reset_cause\);/)
    assert.match(systemSource, /uint8_t mcusr = reset_cause;/)
})

test('jp1 headless RX diagnostics can disable the matrix refresh load after boot', () => {
    const systemSource = fs.readFileSync(systemPath, 'utf8')

    assert.match(systemSource, /#ifdef JP1_DEBUG_HEADLESS_RX/)
    assert.match(systemSource, /display\.disable\(\);/)
})

test('jp1 diagnostics can skip modem bring-up while keeping boot logging alive', () => {
    const systemSource = fs.readFileSync(systemPath, 'utf8')

    assert.match(systemSource, /#ifdef JP1_DEBUG_SKIP_MODEM/)
    assert.match(systemSource, /debuglog::println\("MODEM SKIP"\);/)
})
