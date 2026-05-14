import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const pcbPath = path.join(repoRoot, 'hardware', 'Blinkenstar', 'Blinkenstar.kicad_pcb')
const pcbSource = fs.readFileSync(pcbPath, 'utf8')

/**
 * Return one complete s-expression block starting at the supplied offset.
 *
 * @param {string} source Source text to scan.
 * @param {number} start Offset of the opening parenthesis.
 * @returns {string} Complete s-expression block.
 */
function readSexprBlock(source, start) {
    assert.equal(source[start], '(', 'expected s-expression block to start with an opening parenthesis')

    let depth = 0
    let inString = false
    let escaped = false

    for (let index = start; index < source.length; index++) {
        const char = source[index]

        if (inString) {
            if (escaped) {
                escaped = false
            } else if (char === '\\') {
                escaped = true
            } else if (char === '"') {
                inString = false
            }
            continue
        }

        if (char === '"') {
            inString = true
        } else if (char === '(') {
            depth++
        } else if (char === ')') {
            depth--
            if (depth === 0) {
                return source.slice(start, index + 1)
            }
        }
    }

    throw new Error('unterminated s-expression block')
}

/**
 * Return the footprint block for a reference designator.
 *
 * @param {string} source KiCad PCB source text.
 * @param {string} reference Reference designator to locate.
 * @returns {string} Footprint s-expression block.
 */
function findFootprintByReference(source, reference) {
    const referenceMarker = `(property "Reference" "${reference}"`
    const referenceIndex = source.indexOf(referenceMarker)
    assert.notEqual(referenceIndex, -1, `expected ${reference} footprint reference to exist`)

    const footprintStart = source.lastIndexOf('(footprint ', referenceIndex)
    assert.notEqual(footprintStart, -1, `expected ${reference} to be inside a footprint block`)
    return readSexprBlock(source, footprintStart)
}

/**
 * Return a pad block from a footprint.
 *
 * @param {string} footprint KiCad footprint source text.
 * @param {string} padName Pad number/name to locate.
 * @returns {string} Pad s-expression block.
 */
function findPad(footprint, padName) {
    const padIndex = footprint.indexOf(`(pad "${padName}"`)
    assert.notEqual(padIndex, -1, `expected pad ${padName} to exist`)
    return readSexprBlock(footprint, padIndex)
}

/**
 * Escape text for use in a regular expression.
 *
 * @param {string} value Literal text to escape.
 * @returns {string} Regular-expression-safe text.
 */
function escapeRegExp(value) {
    return value.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
}

/**
 * Verify that the mounted audio jack keeps the transfer input and source ground electrically referenced.
 */
test('audio jack footprint connects the routed input contact and sleeve ground', () => {
    const jack = findFootprintByReference(pcbSource, 'J1')

    assert.match(findPad(jack, 'RING'), new RegExp(`\\(net \\d+ "${escapeRegExp('Net-(J1-PadS)')}"\\)`))
    assert.match(findPad(jack, 'SLEEVE'), /\(net 2 "GND"\)/)
})
