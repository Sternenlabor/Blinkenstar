import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const storageHeaderPath = path.join(repoRoot, 'firmware', 'lib', 'Storage', 'Storage.h')
const storageSourcePath = path.join(repoRoot, 'firmware', 'lib', 'Storage', 'Storage.cpp')
const twiBusHeaderPath = path.join(repoRoot, 'firmware', 'lib', 'TwiBus', 'TwiBus.h')
const twiBusSourcePath = path.join(repoRoot, 'firmware', 'lib', 'TwiBus', 'TwiBus.cpp')

/**
 * Verify that `Storage` no longer owns raw TWI helpers or TWI register programming after the refactor.
 */
test('storage delegates generic bus work to TwiBus', () => {
    const storageHeader = fs.readFileSync(storageHeaderPath, 'utf8')
    const storageSource = fs.readFileSync(storageSourcePath, 'utf8')

    assert.doesNotMatch(storageHeader, /i2c_read\(/)
    assert.doesNotMatch(storageHeader, /i2c_write\(/)
    assert.doesNotMatch(storageSource, /\bTWBR\b|\bTWCR\b|\bTWDR\b|\bTWSR\b/)
    assert.match(storageSource, /twiBus\.enable\(\);/)
    assert.match(storageSource, /twiBus\.read\(/)
    assert.match(storageSource, /twiBus\.write\(/)
})

/**
 * Verify that the STOP helper lives on the `TwiBus` type instead of as a free static helper.
 */
test('twi bus owns its STOP helper as a private class method', () => {
    const twiBusHeader = fs.readFileSync(twiBusHeaderPath, 'utf8')
    const twiBusSource = fs.readFileSync(twiBusSourcePath, 'utf8')

    assert.match(twiBusHeader, /private:\s*[\s\S]*static void stop_\(\);/)
    assert.match(twiBusSource, /void TwiBus::stop_\(\)/)
    assert.doesNotMatch(twiBusSource, /static inline void twiStop_\(\)/)
})

/**
 * Verify that raw TWI transaction helpers stay on `TwiBus` instead of in a file-local namespace.
 */
test('twi bus keeps its remaining transaction helpers on the class', () => {
    const twiBusHeader = fs.readFileSync(twiBusHeaderPath, 'utf8')
    const twiBusSource = fs.readFileSync(twiBusSourcePath, 'utf8')

    assert.match(twiBusHeader, /private:\s*[\s\S]*static Status startRead_\(uint8_t deviceAddress\);/)
    assert.match(twiBusHeader, /private:\s*[\s\S]*static Status startWrite_\(uint8_t deviceAddress\);/)
    assert.match(twiBusHeader, /private:\s*[\s\S]*static uint8_t send_\(uint8_t len, uint8_t \*data\);/)
    assert.match(twiBusHeader, /private:\s*[\s\S]*static uint8_t receive_\(uint8_t len, uint8_t \*data\);/)
    assert.match(twiBusSource, /TwiBus::Status TwiBus::startRead_\(uint8_t deviceAddress\)/)
    assert.match(twiBusSource, /TwiBus::Status TwiBus::startWrite_\(uint8_t deviceAddress\)/)
    assert.match(twiBusSource, /uint8_t TwiBus::send_\(uint8_t len, uint8_t \*data\)/)
    assert.match(twiBusSource, /uint8_t TwiBus::receive_\(uint8_t len, uint8_t \*data\)/)
    assert.doesNotMatch(twiBusSource, /namespace\s*\{/)
})
