#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

out="${1:-work/a26-sd.img}"
size_mb="${2:-64}"
mkdir -p "$(dirname "$out")"

command -v mkfs.vfat >/dev/null
command -v mcopy >/dev/null
rm -f "$out"
dd if=/dev/zero of="$out" bs=1M count="$size_mb" status=none
mkfs.vfat -F 32 -n A26TEST "$out" >/dev/null
export MTOOLS_SKIP_CHECK=1

mmd -i "$out" "::SYSTEM"
mmd -i "$out" "::SYSTEM/PLUG"
mmd -i "$out" "::SYSTEM/SAVER"
mmd -i "$out" "::ROMS"
mmd -i "$out" "::SYSTEM/KERNELS"
plugin="${PLUGIN_GBA:-source/atari2600/prototype.gba}"
mcopy -i "$out" "$plugin" "::SYSTEM/PLUG/a26.gba"
mcopy -i "$out" test-roms/flappy.a26 "::ROMS/flappy.a26"
if [[ -n "${KERNEL_GBA:-}" ]]; then
  mcopy -i "$out" "$KERNEL_GBA" "::SYSTEM/KERNELS/KERNEL.GBA"
fi

if [[ -n "${CONTROL_PLUGIN_GBA:-}" || -n "${CONTROL_PLUGIN_EXT:-}" ||
      -n "${CONTROL_ROM:-}" || -n "${CONTROL_ROM_NAME:-}" ]]; then
  : "${CONTROL_PLUGIN_GBA:?set CONTROL_PLUGIN_GBA}"
  : "${CONTROL_PLUGIN_EXT:?set CONTROL_PLUGIN_EXT, for example nes}"
  : "${CONTROL_ROM:?set CONTROL_ROM}"
  : "${CONTROL_ROM_NAME:?set CONTROL_ROM_NAME, for example snailmaze.nes}"
  if [[ ! "$CONTROL_PLUGIN_EXT" =~ ^[A-Za-z0-9_-]{1,16}$ ||
        "$CONTROL_ROM_NAME" == */* || "$CONTROL_ROM_NAME" == *..* ||
        "$CONTROL_ROM_NAME" != *.* ]]; then
    echo "invalid control plugin extension or ROM filename" >&2
    exit 2
  fi
  ext_lower="${CONTROL_PLUGIN_EXT,,}"
  mcopy -i "$out" "$CONTROL_PLUGIN_GBA" "::SYSTEM/PLUG/${ext_lower}.gba"
  mcopy -i "$out" "$CONTROL_ROM" "::ROMS/${CONTROL_ROM_NAME}"
fi

echo "Created FAT32 SD image: $out"
