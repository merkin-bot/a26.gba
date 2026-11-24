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
mmd -i "$out" "::ROMS"
plugin="${PLUGIN_GBA:-source/atari2600/prototype.gba}"
mcopy -i "$out" "$plugin" "::SYSTEM/PLUG/a26.gba"
mcopy -i "$out" test-roms/flappy.a26 "::ROMS/flappy.a26"

echo "Created FAT32 SD image: $out"
