import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))

function listJsFiles(rootDir) {
    return fs.readdirSync(rootDir, { withFileTypes: true })
        .flatMap((entry) => {
            const fullPath = path.join(rootDir, entry.name)

            if (entry.isDirectory()) {
                return listJsFiles(fullPath)
            }

            return entry.name.endsWith('.js') ? [fullPath] : []
        })
}

test('package scripts target .mjs entry points', () => {
    const packageJson = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'package.json'), 'utf8'))

    assert.equal(packageJson.scripts['tone:test'], 'node scripts/play-sine.mjs')
    assert.equal(packageJson.scripts['transfer:test'], 'node scripts/play-transfer-once.mjs')
})

test('first-party scripts and tests do not keep .js files', () => {
    const repoRoot = path.join(__dirname, '..')
    const jsFiles = [
        ...listJsFiles(path.join(repoRoot, 'scripts')),
        ...listJsFiles(path.join(repoRoot, 'test'))
    ]

    assert.deepEqual(jsFiles, [])
})
