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
const flashScriptPath = path.join(firmwareRoot, 'scripts', 'flash.sh')
const distReadmePath = path.join(firmwareRoot, 'dist', 'README.md')

/**
 * Verify that the legacy `_old` firmware bundle has been removed from the active repo layout.
 */
test('legacy _old firmware archive is removed from the active firmware tree', () => {
    assert.equal(fs.existsSync(path.join(firmwareRoot, '_old')), false)
})

/**
 * Verify that flashing now goes through the maintained firmware script and dist output folder.
 */
test('firmware flash helper builds current images into dist before programming', () => {
    const flashScript = fs.readFileSync(flashScriptPath, 'utf8')

    assert.equal(fs.existsSync(flashScriptPath), true)
    assert.equal(fs.existsSync(distReadmePath), true)
    assert.match(flashScript, /locale="\$\{2:-en\}"/)
    assert.match(flashScript, /artifact_base="firmware_\$\{locale\}"/)
    assert.match(flashScript, /build_env="\$\{env\}"/)
    assert.match(flashScript, /run -c "\$\{project_conf\}" -e "\$\{build_env\}"/)
    assert.match(flashScript, /case "\$\{locale\}" in/)
    assert.match(flashScript, /de\)/)
    assert.match(flashScript, /-DLANG_DE/)
    assert.match(flashScript, /\[env:\$\{build_env\}\]/)
    assert.match(flashScript, /mkdir -p "\$\{dist_dir\}"/)
    assert.match(flashScript, /cp "\$\{build_dir\}\/firmware\.elf" "\$\{dist_dir\}\/\$\{artifact_base\}\.elf"/)
    assert.match(flashScript, /cp "\$\{build_dir\}\/firmware\.hex" "\$\{dist_dir\}\/\$\{artifact_base\}\.hex"/)
    assert.match(flashScript, /-U lfuse:w:0xee:m/)
    assert.match(flashScript, /-U hfuse:w:0xdf:m/)
    assert.match(flashScript, /-U efuse:w:0xff:m/)
    assert.match(flashScript, /-U flash:w:"\$\{dist_dir\}\/\$\{artifact_base\}\.hex":i/)
})

/**
 * Verify that contributor and user-facing docs reference the new dist and flashing layout.
 */
test('docs describe the maintained flash script and dist folder instead of firmware/_old', () => {
    const agents = fs.readFileSync(agentsPath, 'utf8')
    const firmwareDocs = fs.readFileSync(firmwareDocsPath, 'utf8')

    assert.doesNotMatch(agents, /firmware\/_old/)
    assert.match(agents, /firmware\/dist\//)
    assert.match(firmwareDocs, /firmware\/scripts\/flash\.sh/)
    assert.match(firmwareDocs, /firmware\/dist\//)
    assert.match(firmwareDocs, /FUSE|fuse/)
})
