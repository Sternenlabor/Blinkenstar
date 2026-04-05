import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const platformioPath = path.join(__dirname, '..', 'firmware', 'platformio.ini')
const platformioConfig = fs.readFileSync(platformioPath, 'utf8')

/**
 * Return the raw text block for a named PlatformIO environment.
 *
 * @param {string} name Environment name without the `env:` prefix.
 * @returns {string|null} Raw environment block or `null` if it is missing.
 */
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

/**
 * Verify that only the supported consolidated firmware environments remain in the checked-in config.
 */
test('platformio.ini exposes only the consolidated build environments', () => {
    for (const name of ['release', 'debugwire', 'jp1debug', 'hwdiag']) {
        assert.ok(getEnvSection(name), `expected env:${name} to exist`)
    }

    for (const obsolete of ['attiny88', 'debug', 'diag', 'rxdiag', 'rxstore', 'jp1rxdebug', 'jp1rxstore']) {
        assert.equal(getEnvSection(obsolete), null, `expected env:${obsolete} to be removed`)
    }
})

/**
 * Verify that `jp1debug` inherits the release modem path while adding JP1-specific diagnostics.
 */
test('jp1debug is the RX diagnostic build with JP1 serial logging', () => {
    const release = getEnvSection('release')
    const jp1debug = getEnvSection('jp1debug')
    const debugwire = getEnvSection('debugwire')

    assert.ok(release, 'expected env:release to exist')
    assert.ok(jp1debug, 'expected env:jp1debug to exist')
    assert.ok(debugwire, 'expected env:debugwire to exist')
    assert.match(jp1debug, /extends = env:release/)

    for (const flag of ['-DENABLE_MODEM', '-DRX_ALWAYS_ON', '-DMODEM_ADC_CHANNEL=6']) {
        assert.match(release, new RegExp(flag.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')))
    }

    assert.doesNotMatch(release, /-DRX_SLOW_ADC/)
    assert.doesNotMatch(release, /-DMODEM_ACTIVITY_THRESHOLD=/)
    assert.doesNotMatch(release, /-DMODEM_NUMBER_OF_SAMPLES=/)
    assert.doesNotMatch(release, /-DMODEM_BITLEN_THRESHOLD=/)
    assert.doesNotMatch(release, /-DMODEM_BOOT_DELAY_MS=/)
    assert.doesNotMatch(release, /-DNO_BOOT_MESSAGE/)
    assert.match(jp1debug, /-DRX_NO_STORAGE/)
    assert.match(jp1debug, /-DNO_BOOT_MESSAGE/)
    assert.match(jp1debug, /-DRX_POLLING/)
    assert.match(jp1debug, /-DMODEM_DISABLE_FRONTEND_BIAS/)
    assert.match(jp1debug, /-DJP1_DEBUG_SERIAL/)
    assert.match(jp1debug, /-DJP1_DEBUG_NO_HEARTBEAT/)
    assert.match(jp1debug, /-DJP1_DEBUG_RX_EVENTS/)
    assert.match(jp1debug, /-DJP1_DEBUG_HEADLESS_RX/)
    assert.doesNotMatch(jp1debug, /-DJP1_DEBUG_SILENT/)
    assert.match(debugwire, /-DNO_STORED_PATTERN_BOOT_RESTORE/)
})

/**
 * Verify that `jp1debug` keeps the low-RAM no-boot-message policy.
 */
test('jp1debug keeps NO_BOOT_MESSAGE to preserve the low-RAM RX debug path', () => {
    const jp1debug = getEnvSection('jp1debug')

    assert.ok(jp1debug, 'expected env:jp1debug to exist')
    assert.match(jp1debug, /-DNO_BOOT_MESSAGE/)
})

/**
 * Verify that `hwdiag` keeps its button test behavior while still allowing boot-message isolation checks.
 */
test('hwdiag keeps button diagnostics while enabling the boot-message renderer for isolation tests', () => {
    const hwdiag = getEnvSection('hwdiag')

    assert.ok(hwdiag, 'expected env:hwdiag to exist')
    assert.match(hwdiag, /-DDIAG_BUTTONS/)
    assert.match(hwdiag, /-DDIAG_BOOT_MESSAGE/)
})
