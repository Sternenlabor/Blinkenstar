import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const fecHeaderPath = path.join(repoRoot, 'firmware', 'lib', 'Modem', 'FECModem.h')
const fecSourcePath = path.join(repoRoot, 'firmware', 'lib', 'Modem', 'FECModem.cpp')

/**
 * Verify that a decoded second byte is exposed immediately instead of waiting
 * for unrelated future raw modem bytes.
 */
test('fec modem exposes its buffered second decoded byte immediately', () => {
    const fecSource = fs.readFileSync(fecSourcePath, 'utf8')

    assert.match(
        fecSource,
        /if \(state_ == SECOND_BYTE\)\s*(?:\{\s*)?return 1;/,
        'SECOND_BYTE already holds a decoded byte and must not depend on g_modem.available()'
    )
})

/**
 * Verify that starting the FEC layer drops stale raw modem bytes from a prior
 * receive attempt before looking for the next frame.
 */
test('fec modem begin resets stale raw receive bytes', () => {
    const fecHeader = fs.readFileSync(fecHeaderPath, 'utf8')

    assert.match(
        fecHeader,
        /void begin\(\)\s*\{\s*g_modem\.begin\(\);\s*g_modem\.clear\(\);\s*state_ = FIRST_BYTE;\s*\}/,
        'begin() should restart FEC assembly from a clean raw modem queue'
    )
})
