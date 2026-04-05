import Speaker from 'speaker'

/**
 * Build the mono 16-bit speaker configuration used by the bench utilities.
 *
 * @param {number} sampleRate PCM sample rate in hertz.
 * @returns {{channels: number, bitDepth: number, sampleRate: number, signed: boolean, float: boolean}} Speaker options.
 */
export function createMonoSpeakerOptions(sampleRate) {
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

/**
 * Create a mono speaker instance for raw PCM playback.
 *
 * @param {number} sampleRate PCM sample rate in hertz.
 * @param {typeof Speaker} [SpeakerClass=Speaker] Speaker implementation to instantiate.
 * @returns {Speaker} Configured speaker instance.
 */
export function createMonoSpeaker(sampleRate, SpeakerClass = Speaker) {
    return new SpeakerClass(createMonoSpeakerOptions(sampleRate))
}

/**
 * Play a PCM buffer once and resolve when the speaker finishes or closes.
 *
 * @param {Buffer} buffer PCM payload to play.
 * @param {{sampleRate: number, SpeakerClass?: typeof Speaker}} [options={}] Playback options.
 * @returns {Promise<void>} Resolves after playback finishes.
 */
export function playBufferOnce(buffer, { sampleRate, SpeakerClass = Speaker } = {}) {
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

        /**
         * Resolve the playback promise exactly once.
         */
        const finish = () => {
            if (!settled) {
                settled = true
                resolve()
            }
        }

        /**
         * Reject the playback promise exactly once.
         *
         * @param {Error} error Playback failure.
         */
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
