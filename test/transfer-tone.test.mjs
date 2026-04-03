const test = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')

const packageJson = JSON.parse(
    fs.readFileSync(path.join(__dirname, '..', 'package.json'), 'utf8')
)

const {
    createTransferTestPattern,
    encodeTransferPayloads,
    createTransferSamples,
    createTransferPcmBuffer
} = require('../scripts/lib/transfer-tone')

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

test('transfer encoder generates one-shot audio samples and PCM output', () => {
    const pattern = createTransferTestPattern({ token: 'A1B2' })
    const samples = createTransferSamples([pattern])
    const pcm = createTransferPcmBuffer([pattern])

    assert.ok(samples instanceof Float32Array)
    assert.ok(samples.length > 0)
    assert.equal(pcm.length, samples.length * 2)
})

test('package.json exposes the transfer:test script', () => {
    assert.equal(packageJson.scripts['transfer:test'], 'node scripts/play-transfer-once.js')
})
