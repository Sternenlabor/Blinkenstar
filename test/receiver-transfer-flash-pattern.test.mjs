import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const repoRoot = path.join(__dirname, '..')
const receiverHeaderPath = path.join(repoRoot, 'firmware', 'lib', 'Modem', 'Receiver.h')
const receiverSourcePath = path.join(repoRoot, 'firmware', 'lib', 'Modem', 'Receiver.cpp')
const systemHeaderPath = path.join(repoRoot, 'firmware', 'lib', 'System', 'System.h')
const systemSourcePath = path.join(repoRoot, 'firmware', 'lib', 'System', 'System.cpp')

/**
 * Verify that a valid transfer start switches the display to the upstream flashing animation.
 */
test('receiver shows the upstream flashing pattern while a frame is being received', () => {
    const receiverHeader = fs.readFileSync(receiverHeaderPath, 'utf8')
    const receiverSource = fs.readFileSync(receiverSourcePath, 'utf8')

    assert.doesNotMatch(receiverHeader, /void showReceiveActivity\(\);/)
    assert.doesNotMatch(receiverSource, /static uint8_t flashing_pattern_buf\[sizeof\(flashingPattern\)\];/)
    assert.match(receiverSource, /static void showTransferFlashPattern\(\)/)
    assert.doesNotMatch(receiverSource, /void ModemReceiver::showReceiveActivity\(\)/)
    assert.match(receiverSource, /for \(uint8_t i = 0; i < sizeof\(flashingPattern\); \+\+i\)\s*\{\s*display_payload_buf\[i\] = pgm_read_byte\(flashingPattern \+ i\);/s)
    assert.match(receiverSource, /showPayloadBuffer\(display_payload_buf\);/)
    assert.match(receiverSource, /storage\.reset\(\);[\s\S]*?showTransferFlashPattern\(\);/s)
})

/**
 * Verify that release does not replace the active display on raw tone/noise alone.
 */
test('system keeps the flashing cue tied to framed receive progress instead of raw tone presence', () => {
    const systemHeader = fs.readFileSync(systemHeaderPath, 'utf8')
    const systemSource = fs.readFileSync(systemSourcePath, 'utf8')

    assert.doesNotMatch(systemHeader, /receive_activity_cued_/)
    assert.doesNotMatch(systemSource, /modemReceiver\.showReceiveActivity\(\)/)
    assert.doesNotMatch(systemSource, /if \(modem_enabled\)\s*\{\s*#if !defined\(RX_NO_STORAGE\)[\s\S]*?g_modem\.isTonePresent\(\)/s)
})
