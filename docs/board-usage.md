# Board Usage

## Normal Power-Up

On normal power-up, the `release` firmware does one of two things:

- if storage is empty, it scrolls the built-in `Storage is empty` message
- if storage contains saved content, it restores the first stored text or animation

## During Transfer

When a valid transfer starts:

- the board switches to the flashing receive animation used by the original upstream firmware
- transfer activity is still tracked internally through the receiver path
- when the frame completes successfully, the new content is saved and then shown

If a frame does not complete:

- after about four seconds without more framed data, the receiver abandons the partial transfer
- the matrix shows the built-in transmission error message
- the existing stored content remains authoritative until the next successful transfer or manual browse action

## Browsing Stored Content

With stored content present:

- press and release the left button to move forward through stored patterns
- press and release the right button to move backward through stored patterns
- browsing wraps around at the ends
- animations with a finite repeat count now pause and auto-advance after their encoded repeat limit is reached

The browse action happens on release, which avoids fighting the dual-button shutdown gesture.

## Shutdown And Wake

- hold both buttons to start the shutdown animation
- after shutdown, the matrix turns off and the MCU enters deep sleep
- press a button to wake the board
- on wake, the display state is restored instead of returning to a blank frame

## Factory Reset

To erase stored content:

1. disconnect power
2. hold both buttons
3. reconnect power while still holding both buttons
4. keep holding briefly so the startup debounce window can confirm the chord

This clears the EEPROM metadata and the board then boots as empty again.

## Hardware Diagnostic Image

With `hwdiag` flashed:

- idle leaves the display alone so boot-message behavior can be inspected
- left button lights the left outer column
- right button lights the right outer column
- both buttons light both outer columns and the two center pixels

Use this image when you need to separate display/button wiring issues from the normal modem and storage runtime.
