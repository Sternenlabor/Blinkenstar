import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const systemHeaderPath = path.join(repoRoot, 'firmware', 'lib', 'System', 'System.h')
const systemSourcePath = path.join(repoRoot, 'firmware', 'lib', 'System', 'System.cpp')
const mainSourcePath = path.join(repoRoot, 'firmware', 'src', 'main.cpp')
const licensePath = path.join(repoRoot, 'LICENSE')
const symbolTablePath = path.join(repoRoot, 'hardware', 'Blinkenstar', 'sym-lib-table')
const schematicPath = path.join(repoRoot, 'hardware', 'Blinkenstar', 'Blinkenstar.kicad_sch')

/**
 * Verify that the firmware runtime singleton and entrypoints no longer use the legacy project symbol.
 */
test('firmware uses blinkenstar instead of the legacy rocket runtime symbol', () => {
    const systemHeader = fs.readFileSync(systemHeaderPath, 'utf8')
    const systemSource = fs.readFileSync(systemSourcePath, 'utf8')
    const mainSource = fs.readFileSync(mainSourcePath, 'utf8')

    assert.match(systemHeader, /extern System blinkenstar;/)
    assert.match(systemSource, /System blinkenstar;/)
    assert.match(mainSource, /blinkenstar\.initialize\(\);/)
    assert.match(mainSource, /blinkenstar\.loop\(\);/)
    assert.doesNotMatch(systemHeader, /extern System rocket;/)
    assert.doesNotMatch(systemSource, /System rocket;/)
    assert.doesNotMatch(mainSource, /rocket\.(initialize|loop)\(\);/)
})

/**
 * Verify that the checked-in text and KiCad source files no longer carry the legacy project name.
 */
test('text and kicad sources no longer use the legacy blinkenrocket naming', () => {
    const licenseText = fs.readFileSync(licensePath, 'utf8')
    const symbolTable = fs.readFileSync(symbolTablePath, 'utf8')
    const schematic = fs.readFileSync(schematicPath, 'utf8')

    assert.doesNotMatch(licenseText, /Blinkenrocket|blinkenrocket/)
    assert.doesNotMatch(symbolTable, /blinkenrocket_cr2032/)
    assert.doesNotMatch(schematic, /blinkenrocket_cr2032/)
    assert.match(symbolTable, /blinkenstar_cr2032-eagle-import/)
    assert.match(schematic, /blinkenstar_cr2032-eagle-import/)
    assert.match(schematic, /blinkenstar_cr2032:/)
})
