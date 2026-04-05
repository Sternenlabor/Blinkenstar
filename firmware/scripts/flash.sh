#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
firmware_root="$(cd "${script_dir}/.." && pwd)"
env="${1:-release}"
locale="${2:-en}"
artifact_base="firmware_${locale}"
dist_dir="${firmware_root}/dist/${env}"
platformio_bin="${PLATFORMIO_BIN:-${HOME}/.platformio/penv/bin/pio}"
platformio_core_dir="${PLATFORMIO_CORE_DIR:-/tmp/pio-core}"
mcu="${MCU:-attiny88}"
programmer="${PROGRAMMER:-atmelice_isp}"
port="${PORT:-usb}"
project_conf="${firmware_root}/platformio.ini"
build_env="${env}"
temp_conf=''

cleanup() {
    if [[ -n "${temp_conf}" && -f "${temp_conf}" ]]; then
        rm -f "${temp_conf}"
    fi
}

trap cleanup EXIT

case "${locale}" in
    en)
        ;;
    de)
        build_env="${env}_locale_de"
        temp_conf="$(mktemp "${TMPDIR:-/tmp}/blinkenstar-platformio-XXXXXX.ini")"
        cp "${firmware_root}/platformio.ini" "${temp_conf}"
        cat >> "${temp_conf}" <<EOF

[env:${build_env}]
extends = env:${env}
build_flags =
    \${env:${env}.build_flags}
    -DLANG_DE
EOF
        project_conf="${temp_conf}"
        ;;
    *)
        echo "Unsupported locale '${locale}'. Use 'en' or 'de'." >&2
        exit 1
        ;;
esac

build_dir="${firmware_root}/.pio/build/${build_env}"

cd "${firmware_root}"

PLATFORMIO_CORE_DIR="${platformio_core_dir}" "${platformio_bin}" run -c "${project_conf}" -e "${build_env}"

mkdir -p "${dist_dir}"
cp "${build_dir}/firmware.elf" "${dist_dir}/${artifact_base}.elf"
cp "${build_dir}/firmware.hex" "${dist_dir}/${artifact_base}.hex"

avrdude \
    -p "${mcu}" \
    -c "${programmer}" \
    -P "${port}" \
    -V \
    -U lfuse:w:0xee:m \
    -U hfuse:w:0xdf:m \
    -U efuse:w:0xff:m \
    -U flash:w:"${dist_dir}/${artifact_base}.hex":i
