import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const modemPath = path.join(repoRoot, 'firmware', 'lib', 'Modem', 'Modem.cpp')

/**
 * Verify that receive sampling disables the digital input buffer on the active ADC pin.
 */
test('modem disables the digital input buffer on the selected ADC channel while sampling', () => {
    const modemSource = fs.readFileSync(modemPath, 'utf8')

    assert.match(modemSource, /DIDR0\s*\|=\s*adcDigitalInputDisableMask_\(\);/)
    assert.match(modemSource, /DIDR0\s*&=\s*~adcDigitalInputDisableMask_\(\);/)
})

/**
 * Verify that the diagnostic single-shot ADC mode is still present in the modem implementation.
 */
test('modem can switch to manual single-shot ADC conversions for receive diagnostics', () => {
    const modemSource = fs.readFileSync(modemPath, 'utf8')

    assert.match(modemSource, /#ifdef MODEM_SINGLE_SHOT_ADC/)
    assert.match(modemSource, /ADCSRA \|= _BV\(ADSC\);/)
})

/**
 * Verify that single-shot mode immediately arms the next conversion after reading the current sample.
 */
test('single-shot ADC mode restarts the next conversion immediately after reading the current sample', () => {
    const modemSource = fs.readFileSync(modemPath, 'utf8')

    assert.match(
        modemSource,
        /uint16_t sample = ADC;\s*#ifdef MODEM_SINGLE_SHOT_ADC\s*startConversion_\(\);\s*#endif\s*uint16_t delta/s
    )
})
