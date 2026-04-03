const test = require('node:test')
const assert = require('node:assert/strict')
const os = require('node:os')
const path = require('node:path')
const { spawnSync } = require('node:child_process')

test('adaptive modem slicer stays idle on noise and toggles on a strong tone', () => {
    const repoRoot = path.join(__dirname, '..')
    const source = path.join(repoRoot, 'firmware', 'test', 'ActivitySlicerHost.cpp')
    const output = path.join(os.tmpdir(), 'blinkenstar-activity-slicer-host')

    const compile = spawnSync(
        'c++',
        ['-std=c++17', source, '-o', output],
        { cwd: repoRoot, encoding: 'utf8' }
    )

    assert.equal(compile.status, 0, compile.stderr || compile.stdout)

    const run = spawnSync(output, [], { cwd: repoRoot, encoding: 'utf8' })
    assert.equal(run.status, 0, run.stderr || run.stdout)
})
