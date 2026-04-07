import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const firmwareRoot = path.join(repoRoot, 'firmware')
const agentsPath = path.join(repoRoot, 'AGENTS.md')
const firmwareDocsPath = path.join(repoRoot, 'docs', 'firmware.md')
const legacyFlashScriptPath = path.join(firmwareRoot, 'scripts', 'flash.sh')
const flashScriptPath = path.join(firmwareRoot, 'dist', 'flash.sh')
const flashBatchPath = path.join(firmwareRoot, 'dist', 'flash.bat')
const distReadmePath = path.join(firmwareRoot, 'dist', 'README.md')

/**
 * Verify that the legacy `_old` firmware bundle has been removed from the active repo layout.
 */
test('legacy _old firmware archive is removed from the active firmware tree', () => {
    assert.equal(fs.existsSync(path.join(firmwareRoot, '_old')), false)
})

/**
 * Verify that flashing now goes through maintained dist helpers that program existing artifacts.
 */
test('firmware flash helpers program existing dist images without rebuilding', () => {
    const flashScript = fs.readFileSync(flashScriptPath, 'utf8')
    const flashBatch = fs.readFileSync(flashBatchPath, 'utf8')

    assert.equal(fs.existsSync(flashScriptPath), true)
    assert.equal(fs.existsSync(flashBatchPath), true)
    assert.equal(fs.existsSync(legacyFlashScriptPath), false)
    assert.equal(fs.existsSync(distReadmePath), true)
    assert.match(flashScript, /env="\$\{1:-release\}"/)
    assert.match(flashScript, /locale="\$\{2:-en\}"/)
    assert.match(flashScript, /artifact_base="firmware_\$\{locale\}"/)
    assert.match(flashScript, /hex_path="\$\{script_dir\}\/\$\{env\}\/\$\{artifact_base\}\.hex"/)
    assert.match(flashScript, /case "\$\{locale\}" in/)
    assert.match(flashScript, /de\)/)
    assert.match(flashScript, /Missing firmware artifact/)
    assert.doesNotMatch(flashScript, /pio run/)
    assert.doesNotMatch(flashScript, /mktemp/)
    assert.doesNotMatch(flashScript, /-DLANG_DE/)
    assert.match(flashScript, /-U lfuse:w:0xee:m/)
    assert.match(flashScript, /-U hfuse:w:0xdf:m/)
    assert.match(flashScript, /-U efuse:w:0xff:m/)
    assert.match(flashScript, /-U flash:w:"\$\{hex_path\}":i/)

    assert.match(flashBatch, /set "ENV_NAME=release"/)
    assert.match(flashBatch, /set "LOCALE=en"/)
    assert.match(flashBatch, /set "ARTIFACT_BASE=firmware_%LOCALE%"/)
    assert.match(flashBatch, /set "HEX_PATH=%SCRIPT_DIR%\\%ENV_NAME%\\%ARTIFACT_BASE%\.hex"/)
    assert.match(flashBatch, /Missing firmware artifact/)
    assert.doesNotMatch(flashBatch, /pio run/i)
    assert.match(flashBatch, /-U lfuse:w:0xee:m/)
    assert.match(flashBatch, /-U hfuse:w:0xdf:m/)
    assert.match(flashBatch, /-U efuse:w:0xff:m/)
    assert.match(flashBatch, /-U flash:w:"%HEX_PATH%":i/)
})

/**
 * Verify that contributor and user-facing docs reference the new dist and flashing layout.
 */
test('docs describe the maintained flash script and dist folder instead of firmware/_old', () => {
    const agents = fs.readFileSync(agentsPath, 'utf8')
    const firmwareDocs = fs.readFileSync(firmwareDocsPath, 'utf8')
    const distReadme = fs.readFileSync(distReadmePath, 'utf8')

    assert.doesNotMatch(agents, /firmware\/_old/)
    assert.match(agents, /firmware\/dist\//)
    assert.match(agents, /firmware\/dist\/flash\.sh/)
    assert.match(agents, /firmware\/dist\/flash\.bat/)
    assert.match(agents, /already-built|existing .*\.hex/i)
    assert.doesNotMatch(firmwareDocs, /firmware\/scripts\/flash\.sh/)
    assert.match(firmwareDocs, /firmware\/dist\/flash\.sh/)
    assert.match(firmwareDocs, /firmware\/dist\/flash\.bat/)
    assert.match(firmwareDocs, /firmware\/dist\//)
    assert.match(firmwareDocs, /does not build|existing .*\.hex|already-built/i)
    assert.match(firmwareDocs, /FUSE|fuse/)
    assert.match(distReadme, /flash\.sh/)
    assert.match(distReadme, /flash\.bat/)
    assert.match(distReadme, /does not build|existing .*\.hex|already-built/i)
})
