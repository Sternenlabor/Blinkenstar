#!/usr/bin/env node
const { playBufferOnce } = require('./lib/audio-output')

const {
    SAMPLE_RATE,
    createTransferPcmBuffer,
    createTransferTestPattern,
    randomToken
} = require('./lib/transfer-tone')

async function main() {
    const token = randomToken(6)
    const pattern = createTransferTestPattern({ token })
    const pcmBuffer = createTransferPcmBuffer([pattern])

    console.log(`Sending transfer test pattern once: "${pattern.text}"`)

    await playBufferOnce(pcmBuffer, { sampleRate: SAMPLE_RATE })
}

main().catch((error) => {
    console.error(`Transfer playback failed: ${error.message}`)
    process.exitCode = 1
})
