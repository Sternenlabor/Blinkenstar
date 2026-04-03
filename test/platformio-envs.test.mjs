const test = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')

const platformioPath = path.join(__dirname, '..', 'firmware', 'platformio.ini')
const platformioConfig = fs.readFileSync(platformioPath, 'utf8')

function getEnvSection(name) {
    const marker = `[env:${name}]`
    const start = platformioConfig.indexOf(marker)
    if (start === -1) {
        return null
    }

    const remaining = platformioConfig.slice(start)
    const nextEnv = remaining.indexOf('\n[env:', marker.length)
    return nextEnv === -1 ? remaining : remaining.slice(0, nextEnv + 1)
}

test('platformio.ini exposes only the consolidated build environments', () => {
    for (const name of ['release', 'debugwire', 'jp1debug', 'hwdiag']) {
        assert.ok(getEnvSection(name), `expected env:${name} to exist`)
    }

    for (const obsolete of ['attiny88', 'debug', 'diag', 'rxdiag', 'rxstore', 'jp1rxdebug', 'jp1rxstore']) {
        assert.equal(getEnvSection(obsolete), null, `expected env:${obsolete} to be removed`)
    }
})

test('jp1debug is the RX diagnostic build with JP1 serial logging', () => {
    const release = getEnvSection('release')
    const jp1debug = getEnvSection('jp1debug')

    assert.ok(release, 'expected env:release to exist')
    assert.ok(jp1debug, 'expected env:jp1debug to exist')
    assert.match(jp1debug, /extends = env:release/)

    for (const flag of [
        '-DENABLE_MODEM',
        '-DRX_ALWAYS_ON',
        '-DRX_SLOW_ADC',
        '-DMODEM_ACTIVITY_THRESHOLD=30',
        '-DMODEM_NUMBER_OF_SAMPLES=4',
        '-DMODEM_BITLEN_THRESHOLD=3',
        '-DNO_BOOT_MESSAGE'
    ]) {
        assert.match(release, new RegExp(flag.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')))
    }

    assert.match(jp1debug, /-DRX_NO_STORAGE/)
    assert.match(jp1debug, /-DJP1_DEBUG_SERIAL/)
    assert.match(jp1debug, /-DJP1_DEBUG_TONE_DIAG/)
})
