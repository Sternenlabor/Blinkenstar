# Firmware Dist

This directory holds distributable firmware artifacts copied out of PlatformIO build outputs and the maintained flashing helpers.

The maintained flashing helpers are:

- [`./flash.sh`](./flash.sh)
- [`./flash.bat`](./flash.bat)

These helpers:

1. look for an existing locale-tagged `firmware_<locale>.hex` in `firmware/dist/<env>/`
2. stop with an error if the expected artifact is missing
3. program the board and write the expected fuse values

They do not build firmware or copy artifacts into `firmware/dist/`.

Example:

```bash
cd firmware
./dist/flash.sh release
./dist/flash.sh release de
```

On Windows from `firmware\`:

```bat
dist\flash.bat release
dist\flash.bat release de
```
