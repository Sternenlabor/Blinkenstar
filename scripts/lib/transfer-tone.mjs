import { randomBytes } from 'node:crypto'

const SAMPLE_RATE = 48000
const INT16_MAX = 32767

const LEGACY_START = [0xa5, 0xa5, 0xa5, 0x5a]
const LEGACY_BLOCK = [0x0f, 0xf0]
const LEGACY_END = [0x84, 0x84, 0x84]

const MODERN_START = [0x99, 0x99]
const MODERN_BLOCK = [0xa9, 0xa9]
const MODERN_END = [0x84, 0x84]

const HammingLow = [0, 3, 5, 6, 6, 5, 3, 0, 7, 4, 2, 1, 1, 2, 4, 7]
const HammingHigh = [0, 9, 10, 3, 11, 2, 1, 8, 12, 5, 6, 15, 7, 14, 13, 4]

function degreesToRadians(value) {
    return (value * Math.PI) / 180
}

function createLegacySymbols() {
    const shortGap = Array(72).fill(0)
    const longGap = Array(144).fill(0)
    const shortPulse = []
    const longPulse = []

    for (let index = 0; index < 18; index += 1) {
        shortPulse.push((index / 18) * Math.sin(degreesToRadians(10 * index)))
        longPulse.push((index / 18) * Math.sin(degreesToRadians(10 * index)))
    }

    for (let index = 0; index < 36; index += 1) {
        shortPulse.push(Math.sin(degreesToRadians(10 * (index + 18))))
    }

    for (let index = 0; index < 108; index += 1) {
        longPulse.push(Math.sin(degreesToRadians(10 * (index + 18))))
    }

    for (let index = 0; index < 18; index += 1) {
        shortPulse.push(((18 - index) / 18) * Math.sin(degreesToRadians(10 * (index + 54))))
        longPulse.push(((18 - index) / 18) * Math.sin(degreesToRadians(10 * (index + 126))))
    }

    return [
        [shortGap, longGap],
        [shortPulse, longPulse]
    ]
}

const LegacySymbols = createLegacySymbols()
const ModernSyncChunks = [
    Array(17).fill(-1),
    Array(17).fill(1)
]
const ModernSymbols = [
    [Array(3).fill(-1), Array(5).fill(-1)],
    [Array(3).fill(1), Array(5).fill(1)]
]

export function randomToken(length = 6) {
    return randomBytes(Math.ceil(length / 2)).toString('hex').toUpperCase().slice(0, length)
}

export function createTransferTestPattern({
    token = randomToken(),
    speed = 0x0e,
    delay = 0,
    direction = 0,
    repeat = 0
} = {}) {
    return {
        type: 'text',
        text: `TEST ${String(token).toUpperCase()}`,
        speed,
        delay,
        direction,
        repeat
    }
}

function toAsciiBytes(text) {
    return [...Buffer.from(text, 'ascii')]
}

function createTextFrameHeader(text) {
    const length = Buffer.byteLength(text, 'ascii')
    return [0x10 | (length >> 8), length & 0xff]
}

function createLegacyTextHeader(pattern) {
    return [
        ((pattern.speed & 0x0f) << 4) | ((pattern.delay * 2) & 0x0f),
        ((pattern.direction & 0x0f) << 4) | (pattern.repeat & 0x0f)
    ]
}

function createModernTextHeader(pattern) {
    return [
        ((pattern.speed & 0x0f) << 4) | ((pattern.delay * 2) & 0x0f),
        (pattern.direction & 0x0f) << 4
    ]
}

function assertSupportedPattern(pattern) {
    if (!pattern || pattern.type !== 'text') {
        throw new Error('transfer test encoder currently supports text patterns only')
    }
}

function buildLegacyRawBytes(patterns) {
    const bytes = [...LEGACY_START]

    for (const pattern of patterns) {
        assertSupportedPattern(pattern)
        bytes.push(
            ...LEGACY_BLOCK,
            ...createTextFrameHeader(pattern.text),
            ...createLegacyTextHeader(pattern),
            ...toAsciiBytes(pattern.text)
        )
    }

    bytes.push(...LEGACY_END)
    return bytes
}

function buildModernRawBytes(patterns) {
    const bytes = [...MODERN_START]

    for (const pattern of patterns) {
        assertSupportedPattern(pattern)
        bytes.push(
            ...MODERN_BLOCK,
            ...createTextFrameHeader(pattern.text),
            ...createModernTextHeader(pattern),
            ...toAsciiBytes(pattern.text)
        )
    }

    bytes.push(...MODERN_END)
    return bytes
}

function hammingParityByte(firstByte, secondByte) {
    // The firmware consumes 2 data bytes followed by one parity byte built from the two nibbles on each side.
    return (HammingLow[secondByte & 0x0f] ^ HammingHigh[secondByte >> 4]) << 4
        | (HammingLow[firstByte & 0x0f] ^ HammingHigh[firstByte >> 4])
}

function encodeFecBytes(rawBytes) {
    const bytes = rawBytes.slice()
    if (bytes.length % 2 !== 0) {
        bytes.push(0)
    }

    const fecBytes = []
    for (let index = 0; index < bytes.length; index += 2) {
        const firstByte = bytes[index]
        const secondByte = bytes[index + 1]
        fecBytes.push(firstByte, secondByte, hammingParityByte(firstByte, secondByte))
    }

    return fecBytes
}

function pushSegment(target, segment) {
    for (let index = 0; index < segment.length; index += 1) {
        target.push(segment[index])
    }
}

function createLegacySyncSignal(repetitions = 1) {
    const samples = []
    for (let index = 0; index < 360 * repetitions; index += 1) {
        if (index < repetitions / 2) {
            samples.push(Math.sin(degreesToRadians(index / 4)))
        } else {
            samples.push(0)
        }
    }
    return samples
}

function appendLegacyByteSamples(target, byte, state) {
    let workingByte = byte
    let hilo = state

    for (let bitIndex = 0; bitIndex < 8; bitIndex += 1) {
        hilo ^= 1
        pushSegment(target, LegacySymbols[hilo][workingByte & 1])
        workingByte >>= 1
    }

    return hilo
}

function createLegacySamples(fecBytes) {
    const samples = createLegacySyncSignal(200)
    let hilo = 0

    for (const byte of fecBytes) {
        hilo = appendLegacyByteSamples(samples, byte, hilo)
    }

    return samples
}

function createModernSyncSignal(repetitions = 1, initialHilo = 0) {
    const samples = []
    let hilo = initialHilo

    for (let index = 0; index < repetitions; index += 1) {
        hilo ^= 1
        pushSegment(samples, ModernSyncChunks[hilo])
    }

    return { samples, hilo }
}

function appendModernByteSamples(target, byte, state) {
    let workingByte = byte
    let hilo = state

    for (let bitIndex = 0; bitIndex < 8; bitIndex += 1) {
        hilo ^= 1
        pushSegment(target, ModernSymbols[hilo][workingByte & 1])
        workingByte >>= 1
    }

    return hilo
}

function createModernSamples(fecBytes) {
    const sync = createModernSyncSignal(1000, 0)
    const samples = sync.samples
    let hilo = sync.hilo
    let countSinceSync = 0

    for (const byte of fecBytes) {
        hilo = appendModernByteSamples(samples, byte, hilo)
        countSinceSync += 1

        // The alternate framing refreshes sync periodically so the firmware decoder can re-lock on longer transfers.
        if (countSinceSync === 9) {
            const extraSync = createModernSyncSignal(4, hilo)
            pushSegment(samples, extraSync.samples)
            hilo = extraSync.hilo
            countSinceSync = 0
        }
    }

    const endSync = createModernSyncSignal(4, hilo)
    pushSegment(samples, endSync.samples)

    return samples
}

export function encodeTransferPayloads(patterns) {
    const legacyRawBytes = buildLegacyRawBytes(patterns)
    const modernRawBytes = buildModernRawBytes(patterns)

    return {
        legacyRawBytes,
        modernRawBytes,
        legacyFecBytes: encodeFecBytes(legacyRawBytes),
        modernFecBytes: encodeFecBytes(modernRawBytes)
    }
}

export function createTransferSamples(patterns) {
    const payloads = encodeTransferPayloads(patterns)
    const legacySamples = createLegacySamples(payloads.legacyFecBytes)
    const modernSamples = createModernSamples(payloads.modernFecBytes)
    const combined = new Float32Array(legacySamples.length + modernSamples.length)

    combined.set(legacySamples, 0)
    combined.set(modernSamples, legacySamples.length)

    return combined
}

export function createTransferPcmBuffer(patterns) {
    const samples = createTransferSamples(patterns)
    const buffer = Buffer.alloc(samples.length * 2)

    for (let index = 0; index < samples.length; index += 1) {
        const clamped = Math.max(-1, Math.min(1, samples[index]))
        buffer.writeInt16LE(Math.round(clamped * INT16_MAX), index * 2)
    }

    return buffer
}

export { SAMPLE_RATE }
