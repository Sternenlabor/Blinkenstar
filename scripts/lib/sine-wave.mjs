const { Readable } = require('node:stream')

const TWO_PI = Math.PI * 2
const INT16_MAX = 32767

function createSineBuffer({ sampleRate, frequencyHz, amplitude, sampleCount, phase = 0 }) {
    if (!Number.isInteger(sampleCount) || sampleCount <= 0) {
        throw new RangeError('sampleCount must be a positive integer')
    }

    if (!(sampleRate > 0)) {
        throw new RangeError('sampleRate must be positive')
    }

    if (!(frequencyHz > 0)) {
        throw new RangeError('frequencyHz must be positive')
    }

    if (!(amplitude > 0 && amplitude <= 1)) {
        throw new RangeError('amplitude must be between 0 and 1')
    }

    const buffer = Buffer.alloc(sampleCount * 2)
    const phaseStep = (TWO_PI * frequencyHz) / sampleRate
    let nextPhase = phase

    for (let index = 0; index < sampleCount; index += 1) {
        const sample = Math.round(Math.sin(nextPhase) * amplitude * INT16_MAX)
        buffer.writeInt16LE(sample, index * 2)
        nextPhase += phaseStep

        if (nextPhase >= TWO_PI) {
            nextPhase %= TWO_PI
        }
    }

    return { buffer, phase: nextPhase }
}

class SineWavePcmStream extends Readable {
    constructor({
        sampleRate = 48000,
        frequencyHz = 1000,
        amplitude = 0.2,
        samplesPerChunk = 1024
    } = {}) {
        super()
        this.sampleRate = sampleRate
        this.frequencyHz = frequencyHz
        this.amplitude = amplitude
        this.samplesPerChunk = samplesPerChunk
        this.phase = 0
        this.stopped = false
    }

    stop() {
        if (this.stopped) {
            return
        }

        this.stopped = true
        this.push(null)
    }

    _read() {
        if (this.stopped) {
            this.push(null)
            return
        }

        const { buffer, phase } = createSineBuffer({
            sampleRate: this.sampleRate,
            frequencyHz: this.frequencyHz,
            amplitude: this.amplitude,
            sampleCount: this.samplesPerChunk,
            phase: this.phase
        })

        // Preserve phase across chunks so the stream stays continuous instead of clicking at buffer edges.
        this.phase = phase
        this.push(buffer)
    }
}

module.exports = {
    createSineBuffer,
    SineWavePcmStream
}
