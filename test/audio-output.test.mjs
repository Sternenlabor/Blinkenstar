import test from 'node:test'
import assert from 'node:assert/strict'
import { EventEmitter } from 'node:events'

import { createMonoSpeakerOptions, playBufferOnce } from '../scripts/lib/audio-output.mjs'

/**
 * Minimal event-emitting speaker double used to test playback logic without a real audio device.
 */
class FakeSpeaker extends EventEmitter {
    /**
     * Capture the speaker options used by the code under test.
     *
     * @param {object} options Instantiation options.
     */
    constructor(options) {
        super()
        this.options = options
        this.endedBuffer = null
    }

    /**
     * Record the submitted buffer and emit a synthetic finish event.
     *
     * @param {Buffer} buffer PCM data written by the helper.
     */
    end(buffer) {
        this.endedBuffer = buffer
        process.nextTick(() => {
            this.emit('finish')
        })
    }
}

/**
 * Verify that the speaker helper produces the mono signed-PCM configuration expected by the CLI tools.
 */
test('createMonoSpeakerOptions returns the expected speaker configuration', () => {
    assert.deepEqual(createMonoSpeakerOptions(44100), {
        channels: 1,
        bitDepth: 16,
        sampleRate: 44100,
        signed: true,
        float: false
    })
})

/**
 * Verify that invalid sample rates are rejected before speaker construction.
 */
test('createMonoSpeakerOptions rejects invalid sample rates', () => {
    assert.throws(() => createMonoSpeakerOptions(0), /sampleRate must be a positive integer/)
    assert.throws(() => createMonoSpeakerOptions(44.1), /sampleRate must be a positive integer/)
})

/**
 * Verify that successful playback writes the caller-provided PCM buffer and resolves.
 */
test('playBufferOnce writes the provided PCM buffer and resolves on finish', async () => {
    const buffer = Buffer.from([0x01, 0x02, 0x03, 0x04])
    let speakerInstance = null

    await playBufferOnce(buffer, {
        sampleRate: 32000,
        SpeakerClass: class extends FakeSpeaker {
            constructor(options) {
                super(options)
                speakerInstance = this
            }
        }
    })

    assert.ok(speakerInstance)
    assert.deepEqual(speakerInstance.options, createMonoSpeakerOptions(32000))
    assert.equal(speakerInstance.endedBuffer, buffer)
})

/**
 * Verify that construction failures propagate as rejected playback promises.
 */
test('playBufferOnce rejects when speaker creation fails', async () => {
    await assert.rejects(
        playBufferOnce(Buffer.alloc(2), {
            sampleRate: 32000,
            SpeakerClass: class BrokenSpeaker {
                constructor() {
                    throw new Error('speaker unavailable')
                }
            }
        }),
        /speaker unavailable/
    )
})

/**
 * Verify that runtime speaker errors reject playback even after instantiation succeeds.
 */
test('playBufferOnce rejects when the speaker emits an error', async () => {
    await assert.rejects(
        playBufferOnce(Buffer.alloc(2), {
            sampleRate: 32000,
            SpeakerClass: class ErrorSpeaker extends FakeSpeaker {
                end() {
                    process.nextTick(() => {
                        this.emit('error', new Error('write failed'))
                    })
                }
            }
        }),
        /write failed/
    )
})
