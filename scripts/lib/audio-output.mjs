const Speaker = require('speaker')

function createMonoSpeakerOptions(sampleRate) {
    if (!Number.isInteger(sampleRate) || sampleRate <= 0) {
        throw new RangeError('sampleRate must be a positive integer')
    }

    return {
        channels: 1,
        bitDepth: 16,
        sampleRate,
        signed: true,
        float: false
    }
}

function createMonoSpeaker(sampleRate, SpeakerClass = Speaker) {
    return new SpeakerClass(createMonoSpeakerOptions(sampleRate))
}

function playBufferOnce(buffer, { sampleRate, SpeakerClass = Speaker } = {}) {
    return new Promise((resolve, reject) => {
        let settled = false
        let speaker

        try {
            // Allow Speaker injection so the CLI can be tested without opening a real audio device.
            speaker = createMonoSpeaker(sampleRate, SpeakerClass)
        } catch (error) {
            reject(error)
            return
        }

        const finish = () => {
            if (!settled) {
                settled = true
                resolve()
            }
        }

        const fail = (error) => {
            if (!settled) {
                settled = true
                reject(error)
            }
        }

        speaker.once('error', fail)
        speaker.once('close', finish)
        speaker.once('finish', finish)
        speaker.end(buffer)
    })
}

module.exports = {
    createMonoSpeakerOptions,
    createMonoSpeaker,
    playBufferOnce
}
