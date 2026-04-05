import test from 'node:test'
import assert from 'node:assert/strict'

import { createSineBuffer } from '../scripts/lib/sine-wave.mjs'

/**
 * Verify that the sine generator returns exactly one 16-bit sample per requested sample slot.
 */
test('createSineBuffer returns the requested number of 16-bit PCM samples', () => {
    const { buffer } = createSineBuffer({
        sampleRate: 48000,
        frequencyHz: 1000,
        amplitude: 0.25,
        sampleCount: 1024,
        phase: 0
    })

    assert.equal(buffer.length, 1024 * 2)
})

/**
 * Verify that the sine generator never overflows the signed 16-bit PCM range.
 */
test('createSineBuffer keeps samples inside the signed 16-bit range', () => {
    const { buffer } = createSineBuffer({
        sampleRate: 48000,
        frequencyHz: 1000,
        amplitude: 1,
        sampleCount: 2048,
        phase: 0
    })

    for (let offset = 0; offset < buffer.length; offset += 2) {
        const sample = buffer.readInt16LE(offset)
        assert.ok(sample >= -32767)
        assert.ok(sample <= 32767)
    }
})

/**
 * Verify that carrying phase between chunks produces a continuous waveform instead of restarting at zero.
 */
test('createSineBuffer preserves phase across adjacent chunks', () => {
    const first = createSineBuffer({
        sampleRate: 8000,
        frequencyHz: 1000,
        amplitude: 1,
        sampleCount: 4,
        phase: 0
    })

    const second = createSineBuffer({
        sampleRate: 8000,
        frequencyHz: 1000,
        amplitude: 1,
        sampleCount: 4,
        phase: first.phase
    })

    assert.deepEqual(
        [0, 2, 4, 6].map((offset) => first.buffer.readInt16LE(offset)),
        [0, 23170, 32767, 23170]
    )

    assert.deepEqual(
        [0, 2, 4, 6].map((offset) => second.buffer.readInt16LE(offset)),
        [0, -23170, -32767, -23170]
    )
})
