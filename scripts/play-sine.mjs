#!/usr/bin/env node
import { pipeline } from 'node:stream'

import { createMonoSpeaker } from './lib/audio-output.mjs'
import { SineWavePcmStream } from './lib/sine-wave.mjs'

const sampleRate = 48000
const toneStream = new SineWavePcmStream({
    sampleRate,
    frequencyHz: 1000,
    amplitude: 0.2,
    samplesPerChunk: 1024
})

let speaker

try {
    speaker = createMonoSpeaker(sampleRate)
} catch (error) {
    console.error(`Unable to start speaker output: ${error.message}`)
    process.exit(1)
}

const shutdown = () => {
    toneStream.stop()
}

process.on('SIGINT', shutdown)
process.on('SIGTERM', shutdown)

console.log('Playing 1 kHz sine wave. Press Ctrl+C to stop.')

pipeline(toneStream, speaker, (error) => {
    if (error && error.code !== 'ERR_STREAM_PREMATURE_CLOSE') {
        console.error(`Tone playback failed: ${error.message}`)
        process.exitCode = 1
    }
})
