import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')

/**
 * Read one repository file as UTF-8 text.
 *
 * @param {string} relativePath Repo-relative file path.
 * @returns {string} File contents.
 */
function readRepoFile(relativePath) {
    return fs.readFileSync(path.join(repoRoot, relativePath), 'utf8')
}

/**
 * Verify that the display restores the upstream animation pause/repeat/autoskip rules.
 */
function displayRestoresUpstreamAnimationBoundaryHandling() {
    const displayHeader = readRepoFile(path.join('firmware', 'lib', 'Display', 'Display.h'))
    const displaySource = readRepoFile(path.join('firmware', 'lib', 'Display', 'Display.cpp'))
    const systemHeader = readRepoFile(path.join('firmware', 'lib', 'System', 'System.h'))
    const systemSource = readRepoFile(path.join('firmware', 'lib', 'System', 'System.cpp'))

    assert.match(systemHeader, /void handleAnimationRepeat\(\);/)
    assert.match(systemSource, /void System::handleAnimationRepeat\(\)/)
    assert.match(systemSource, /modemReceiver\.showStoredPattern\(current_pattern_index_\);/)
    assert.match(displayHeader, /bool consumeAnimationRepeatRequest\(\);/)
    assert.match(displayHeader, /uint8_t repeat_cnt;/)
    assert.match(displaySource, /repeat_cnt = 0;/)
    assert.match(displaySource, /if \(current_anim->delay > 0\)\s*\{\s*status = PAUSED;\s*update_threshold = 244;/s)
    assert.match(displaySource, /repeat_advance_requested_ = true;/)
    assert.match(
        systemSource,
        /display\.update\(\);\s*#if defined\(ENABLE_MODEM\) && !defined\(RX_NO_STORAGE\)\s*if \(display\.consumeAnimationRepeatRequest\(\)\)\s*\{\s*handleAnimationRepeat\(\);/s
    )
}

/**
 * Verify that interrupted transfers time out and restore the upstream timeout message.
 */
function receiverRestoresUpstreamInterruptedTransferTimeout() {
    const receiverHeader = readRepoFile(path.join('firmware', 'lib', 'Modem', 'Receiver.h'))
    const receiverSource = readRepoFile(path.join('firmware', 'lib', 'Modem', 'Receiver.cpp'))

    assert.match(receiverHeader, /FRAME_TIMEOUT_MS = 4000UL/)
    assert.match(receiverHeader, /unsigned long frame_timeout_at_ms_ = 0;/)
    assert.match(receiverSource, /frame_timeout_at_ms_ = now_ms \+ FRAME_TIMEOUT_MS;/)
    assert.match(receiverSource, /if \(state_ != START1 && frame_timeout_at_ms_ != 0 && \(long\)\(now_ms - frame_timeout_at_ms_\) >= 0\)/)
    assert.match(receiverSource, /showTimeoutPattern\(\);/)
    assert.match(receiverSource, /state_ = START1;[\s\S]*frame_payload_complete_ = false;[\s\S]*fecModem\.clear\(\);/s)
}

test('display restores the upstream end-of-animation pause, repeat, and autoskip handling', displayRestoresUpstreamAnimationBoundaryHandling)
test('receiver restores the upstream interrupted-transfer timeout recovery', receiverRestoresUpstreamInterruptedTransferTimeout)
