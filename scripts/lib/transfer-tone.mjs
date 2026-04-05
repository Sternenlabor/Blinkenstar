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

/**
 * Convert degrees to radians for the legacy waveform builder.
 *
 * @param {number} value Angle in degrees.
 * @returns {number} Angle in radians.
 */
function degreesToRadians(value) {
    return (value * Math.PI) / 180
}

/**
 * Precompute the legacy short/long pulse shapes used by the original transfer format.
 *
 * @returns {number[][][]} Legacy symbol lookup table indexed by phase and bit value.
 */
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

/**
 * Generate a random uppercase hexadecimal token for transfer test messages.
 *
 * @param {number} [length=6] Token length in characters.
 * @returns {string} Random token.
 */
export function randomToken(length = 6) {
    return randomBytes(Math.ceil(length / 2)).toString('hex').toUpperCase().slice(0, length)
}

/**
 * Build a single text pattern payload for transfer testing.
 *
 * @param {{token?: string, speed?: number, delay?: number, direction?: number, repeat?: number}} [options={}] Pattern fields.
 * @returns {{type: string, text: string, speed: number, delay: number, direction: number, repeat: number}} Transfer pattern.
 */
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

/**
 * Convert ASCII text into raw byte values.
 *
 * @param {string} text Input text.
 * @returns {number[]} ASCII byte array.
 */
function toAsciiBytes(text) {
    return [...Buffer.from(text, 'ascii')]
}

/**
 * Build the common two-byte text frame header.
 *
 * @param {string} text Pattern text payload.
 * @returns {number[]} Two-byte length header.
 */
function createTextFrameHeader(text) {
    const length = Buffer.byteLength(text, 'ascii')
    return [0x10 | (length >> 8), length & 0xff]
}

/**
 * Build the legacy metadata bytes for a text pattern.
 *
 * @param {{speed: number, delay: number, direction: number, repeat: number}} pattern Pattern metadata.
 * @returns {number[]} Legacy metadata bytes.
 */
function createLegacyTextHeader(pattern) {
    return [
        ((pattern.speed & 0x0f) << 4) | ((pattern.delay * 2) & 0x0f),
        ((pattern.direction & 0x0f) << 4) | (pattern.repeat & 0x0f)
    ]
}

/**
 * Build the alternate-format metadata bytes for a text pattern.
 *
 * @param {{speed: number, delay: number, direction: number}} pattern Pattern metadata.
 * @returns {number[]} Alternate metadata bytes.
 */
function createModernTextHeader(pattern) {
    return [
        ((pattern.speed & 0x0f) << 4) | ((pattern.delay * 2) & 0x0f),
        (pattern.direction & 0x0f) << 4
    ]
}

/**
 * Reject pattern types that the transfer test encoder cannot serialize.
 *
 * @param {{type?: string}|undefined} pattern Candidate pattern object.
 */
function assertSupportedPattern(pattern) {
    if (!pattern || pattern.type !== 'text') {
        throw new Error('transfer test encoder currently supports text patterns only')
    }
}

/**
 * Build the raw legacy transfer frame bytes for one or more patterns.
 *
 * @param {Array<{type: string, text: string, speed: number, delay: number, direction: number, repeat: number}>} patterns Patterns to encode.
 * @returns {number[]} Legacy frame bytes before FEC.
 */
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

/**
 * Build the raw alternate transfer frame bytes for one or more patterns.
 *
 * @param {Array<{type: string, text: string, speed: number, delay: number, direction: number, repeat: number}>} patterns Patterns to encode.
 * @returns {number[]} Alternate frame bytes before FEC.
 */
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

/**
 * Compute the combined parity byte for a pair of raw payload bytes.
 *
 * @param {number} firstByte First payload byte.
 * @param {number} secondByte Second payload byte.
 * @returns {number} Packed Hamming parity byte.
 */
function hammingParityByte(firstByte, secondByte) {
    // The firmware consumes 2 data bytes followed by one parity byte built from the two nibbles on each side.
    return (HammingLow[secondByte & 0x0f] ^ HammingHigh[secondByte >> 4]) << 4
        | (HammingLow[firstByte & 0x0f] ^ HammingHigh[firstByte >> 4])
}

/**
 * Expand raw transfer bytes into the byte stream expected by the modem FEC decoder.
 *
 * @param {number[]} rawBytes Raw frame bytes.
 * @returns {number[]} Raw bytes plus parity bytes.
 */
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

/**
 * Append one sample segment into a mutable sample array.
 *
 * @param {number[]} target Output sample array.
 * @param {number[]} segment Segment to append.
 */
function pushSegment(target, segment) {
    for (let index = 0; index < segment.length; index += 1) {
        target.push(segment[index])
    }
}

/**
 * Create the long sync burst used at the start of legacy transfers.
 *
 * @param {number} [repetitions=1] Number of sync repetitions.
 * @returns {number[]} Sync samples.
 */
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

/**
 * Append one legacy-framed byte worth of samples.
 *
 * @param {number[]} target Output sample array.
 * @param {number} byte Byte to append.
 * @param {number} state Current high/low phase state.
 * @returns {number} Updated phase state.
 */
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

/**
 * Convert FEC bytes into the legacy transfer waveform.
 *
 * @param {number[]} fecBytes Encoded transfer bytes.
 * @returns {number[]} Legacy waveform samples.
 */
function createLegacySamples(fecBytes) {
    const samples = createLegacySyncSignal(200)
    let hilo = 0

    for (const byte of fecBytes) {
        hilo = appendLegacyByteSamples(samples, byte, hilo)
    }

    return samples
}

/**
 * Create the alternating sync run used by the alternate framing mode.
 *
 * @param {number} [repetitions=1] Number of sync chunks.
 * @param {number} [initialHilo=0] Initial phase state.
 * @returns {{samples: number[], hilo: number}} Sync samples and the resulting phase state.
 */
function createModernSyncSignal(repetitions = 1, initialHilo = 0) {
    const samples = []
    let hilo = initialHilo

    for (let index = 0; index < repetitions; index += 1) {
        hilo ^= 1
        pushSegment(samples, ModernSyncChunks[hilo])
    }

    return { samples, hilo }
}

/**
 * Append one alternate-framed byte worth of samples.
 *
 * @param {number[]} target Output sample array.
 * @param {number} byte Byte to append.
 * @param {number} state Current high/low phase state.
 * @returns {number} Updated phase state.
 */
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

/**
 * Convert FEC bytes into the alternate transfer waveform.
 *
 * @param {number[]} fecBytes Encoded transfer bytes.
 * @returns {number[]} Alternate waveform samples.
 */
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

/**
 * Build both raw and FEC-encoded transfer payload variants for the supplied patterns.
 *
 * @param {Array<{type: string, text: string, speed: number, delay: number, direction: number, repeat: number}>} patterns Patterns to encode.
 * @returns {{legacyRawBytes: number[], modernRawBytes: number[], legacyFecBytes: number[], modernFecBytes: number[]}} Encoded payload variants.
 */
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

/**
 * Create a floating-point transfer waveform that concatenates legacy and alternate payloads.
 *
 * @param {Array<{type: string, text: string, speed: number, delay: number, direction: number, repeat: number}>} patterns Patterns to encode.
 * @returns {Float32Array} Combined normalized waveform samples.
 */
export function createTransferSamples(patterns) {
    const payloads = encodeTransferPayloads(patterns)
    const legacySamples = createLegacySamples(payloads.legacyFecBytes)
    const modernSamples = createModernSamples(payloads.modernFecBytes)
    const combined = new Float32Array(legacySamples.length + modernSamples.length)

    combined.set(legacySamples, 0)
    combined.set(modernSamples, legacySamples.length)

    return combined
}

/**
 * Convert the generated transfer waveform into signed 16-bit PCM.
 *
 * @param {Array<{type: string, text: string, speed: number, delay: number, direction: number, repeat: number}>} patterns Patterns to encode.
 * @returns {Buffer} Little-endian signed 16-bit PCM buffer.
 */
export function createTransferPcmBuffer(patterns) {
    const samples = createTransferSamples(patterns)
    const buffer = Buffer.alloc(samples.length * 2)

    for (let index = 0; index < samples.length; index += 1) {
        // Clamp defensively so any future waveform tweaks cannot overflow PCM conversion.
        const clamped = Math.max(-1, Math.min(1, samples[index]))
        buffer.writeInt16LE(Math.round(clamped * INT16_MAX), index * 2)
    }

    return buffer
}

export { SAMPLE_RATE }
