import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const displaySourcePath = path.join(repoRoot, 'firmware', 'lib', 'Display', 'Display.cpp')

test('display refresh period matches the upstream 256 microsecond multiplex timing', () => {
    const displaySource = fs.readFileSync(displaySourcePath, 'utf8')

    assert.match(displaySource, /timer\.initialize\(256\);/)
})
