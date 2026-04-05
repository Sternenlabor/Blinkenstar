# Firmware Dist

This directory holds distributable firmware artifacts copied out of PlatformIO build outputs.

The maintained flashing helper is:

- [`../scripts/flash.sh`](../scripts/flash.sh)

That script:

1. builds a selected firmware environment
2. copies locale-tagged `firmware_<locale>.elf` and `firmware_<locale>.hex` into `firmware/dist/<env>/`
3. programs the board and writes the expected fuse values

Example:

```bash
cd firmware
./scripts/flash.sh release
./scripts/flash.sh release de
```
