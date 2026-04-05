import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import {
    createTransferTestPattern,
    encodeTransferPayloads,
    createTransferSamples,
    createTransferPcmBuffer
} from '../scripts/lib/transfer-tone.mjs'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const packageJson = JSON.parse(
    fs.readFileSync(path.join(__dirname, '..', 'package.json'), 'utf8')
)

/**
 * Verify that both transfer formats wrap the text payload in the expected framing bytes.
 */
test('transfer encoder frames both legacy and alternate transfer payloads around the token text', () => {
    const pattern = createTransferTestPattern({ token: 'ZX4K2' })
    const payloads = encodeTransferPayloads([pattern])

    assert.deepEqual(payloads.legacyRawBytes.slice(0, 4), [0xa5, 0xa5, 0xa5, 0x5a])
    assert.deepEqual(payloads.legacyRawBytes.slice(-3), [0x84, 0x84, 0x84])
    assert.deepEqual(payloads.modernRawBytes.slice(0, 2), [0x99, 0x99])
    assert.deepEqual(payloads.modernRawBytes.slice(-2), [0x84, 0x84])

    const tokenBytes = [...Buffer.from('TEST ZX4K2', 'ascii')]
    for (const byte of tokenBytes) {
        assert.ok(payloads.legacyRawBytes.includes(byte), `legacy payload missing 0x${byte.toString(16)}`)
        assert.ok(payloads.modernRawBytes.includes(byte), `modern payload missing 0x${byte.toString(16)}`)
    }
})

/**
 * Verify that the transfer helper emits both normalized samples and signed PCM output.
 */
test('transfer encoder generates one-shot audio samples and PCM output', () => {
    const pattern = createTransferTestPattern({ token: 'A1B2' })
    const samples = createTransferSamples([pattern])
    const pcm = createTransferPcmBuffer([pattern])

    assert.ok(samples instanceof Float32Array)
    assert.ok(samples.length > 0)
    assert.equal(pcm.length, samples.length * 2)
})

/**
 * Verify that the npm script still points at the one-shot transfer entrypoint.
 */
test('package.json exposes the transfer:test script', () => {
    assert.equal(packageJson.scripts['transfer:test'], 'node scripts/play-transfer-once.mjs')
})
