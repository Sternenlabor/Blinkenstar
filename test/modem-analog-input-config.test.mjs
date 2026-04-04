import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const modemPath = path.join(repoRoot, 'firmware', 'lib', 'Modem', 'Modem.cpp')

test('modem disables the digital input buffer on the selected ADC channel while sampling', () => {
    const modemSource = fs.readFileSync(modemPath, 'utf8')

    assert.match(modemSource, /DIDR0\s*\|=\s*adcDigitalInputDisableMask_\(\);/)
    assert.match(modemSource, /DIDR0\s*&=\s*~adcDigitalInputDisableMask_\(\);/)
})

test('modem can switch to manual single-shot ADC conversions for receive diagnostics', () => {
    const modemSource = fs.readFileSync(modemPath, 'utf8')

    assert.match(modemSource, /#ifdef MODEM_SINGLE_SHOT_ADC/)
    assert.match(modemSource, /ADCSRA \|= _BV\(ADSC\);/)
})

test('single-shot ADC mode restarts the next conversion immediately after reading the current sample', () => {
    const modemSource = fs.readFileSync(modemPath, 'utf8')

    assert.match(
        modemSource,
        /uint16_t sample = ADC;\s*#ifdef MODEM_SINGLE_SHOT_ADC\s*startConversion_\(\);\s*#endif\s*uint16_t delta/s
    )
})
