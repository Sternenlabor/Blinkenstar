#!/usr/bin/env node
const { pipeline } = require('node:stream');
const Speaker = require('speaker');

const { SineWavePcmStream } = require('./lib/sine-wave');

const sampleRate = 48000;
const toneStream = new SineWavePcmStream({
  sampleRate,
  frequencyHz: 1000,
  amplitude: 0.2,
  samplesPerChunk: 1024,
});

let speaker;

try {
  speaker = new Speaker({
    channels: 1,
    bitDepth: 16,
    sampleRate,
    signed: true,
    float: false,
  });
} catch (error) {
  console.error(`Unable to start speaker output: ${error.message}`);
  process.exit(1);
}

const shutdown = () => {
  toneStream.stop();
};

process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);

console.log('Playing 1 kHz sine wave. Press Ctrl+C to stop.');

pipeline(toneStream, speaker, (error) => {
  if (error && error.code !== 'ERR_STREAM_PREMATURE_CLOSE') {
    console.error(`Tone playback failed: ${error.message}`);
    process.exitCode = 1;
  }
});
