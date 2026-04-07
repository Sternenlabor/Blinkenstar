#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
env="${1:-release}"
locale="${2:-en}"
artifact_base="firmware_${locale}"
hex_path="${script_dir}/${env}/${artifact_base}.hex"
avrdude_bin="${AVRDUDE_BIN:-avrdude}"
mcu="${MCU:-attiny88}"
programmer="${PROGRAMMER:-atmelice_isp}"
port="${PORT:-usb}"

case "${locale}" in
    en | de)
        ;;
    *)
        echo "Unsupported locale '${locale}'. Use 'en' or 'de'." >&2
        exit 1
        ;;
esac

if [[ ! -f "${hex_path}" ]]; then
    echo "Missing firmware artifact '${hex_path}'. Build or copy the desired .hex into firmware/dist/${env}/ first." >&2
    exit 1
fi

"${avrdude_bin}" \
    -p "${mcu}" \
    -c "${programmer}" \
    -P "${port}" \
    -V \
    -U lfuse:w:0xee:m \
    -U hfuse:w:0xdf:m \
    -U efuse:w:0xff:m \
    -U flash:w:"${hex_path}":i
