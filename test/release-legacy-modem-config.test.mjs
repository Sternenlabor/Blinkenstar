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
 * Verify that the normal release build matches the known-good consolidated release modem flags.
 */
test('release keeps the known-good consolidated modem flags without extra RX tuning overrides', () => {
    const release = getEnvSection('release')

    assert.ok(release, 'expected env:release to exist')
    assert.doesNotMatch(release, /-DRX_SLOW_ADC/)
    assert.doesNotMatch(release, /-DMODEM_ACTIVITY_THRESHOLD=/)
    assert.doesNotMatch(release, /-DMODEM_NUMBER_OF_SAMPLES=/)
    assert.doesNotMatch(release, /-DMODEM_BITLEN_THRESHOLD=/)
})
