#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

out="${1:-work/a26-sd.img}"
size_mb="${2:-64}"
filesystem="${3:-fat32}"
mkdir -p "$(dirname "$out")"

rm -f "$out"
truncate -s "$((size_mb * 1024 * 1024))" "$out"
if [[ "$filesystem" == fat32 ]]; then
  command -v mkfs.vfat >/dev/null
  command -v mcopy >/dev/null
  mkfs.vfat -F 32 -n A26TEST "$out" >/dev/null
  export MTOOLS_SKIP_CHECK=1
  image_mkdir() { mmd -i "$out" "::$1"; }
  image_copy() { mcopy -i "$out" "$1" "::$2"; }
elif [[ "$filesystem" == exfat ]]; then
  command -v mkfs.exfat >/dev/null
  command -v fsck.exfat >/dev/null
  command -v losetup >/dev/null
  command -v mount.exfat-fuse >/dev/null
  command -v fusermount3 >/dev/null
  mount_dir="$(mktemp -d "${TMPDIR:-/tmp}/a26-exfat.XXXXXX")"
  loop_dev=""
  cleanup_exfat() {
    if mountpoint -q "$mount_dir"; then sudo umount "$mount_dir" || true; fi
    if [[ -n "$loop_dev" ]]; then sudo losetup -d "$loop_dev" || true; fi
    rmdir "$mount_dir" 2>/dev/null || true
  }
  trap cleanup_exfat EXIT
  mkfs.exfat -L A26TEST -c 32K "$out" >/dev/null
  loop_dev="$(sudo losetup --find --show "$out")"
  sudo mount.exfat-fuse "$loop_dev" "$mount_dir"
  image_mkdir() { mkdir -p "$mount_dir/$1"; }
  image_copy() { mkdir -p "$mount_dir/$(dirname "$2")"; cp "$1" "$mount_dir/$2"; }
else
  echo "filesystem must be fat32 or exfat" >&2
  exit 2
fi

image_mkdir SYSTEM
image_mkdir SYSTEM/PLUG
image_mkdir SYSTEM/SAVER
image_mkdir ROMS
plugin="${PLUGIN_GBA:-source/atari2600/prototype.gba}"
image_copy "$plugin" SYSTEM/PLUG/a26.gba
image_copy test-roms/flappy.a26 ROMS/flappy.a26
if [[ -n "${EZ_THEME_STYLE_BIN:-}" ]]; then
  if [[ "${EZ_THEME_STYLE_BIN##*.}" != "bin" && "${EZ_THEME_STYLE_BIN##*.}" != "BIN" ]]; then
    echo "EZ_THEME_STYLE_BIN must name a .bin EZ-FLASH theme style" >&2
    exit 2
  fi
  image_mkdir SYSTEM/KERNELS
  image_copy "$EZ_THEME_STYLE_BIN" "SYSTEM/KERNELS/$(basename "$EZ_THEME_STYLE_BIN")"
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
  image_copy "$CONTROL_PLUGIN_GBA" "SYSTEM/PLUG/${ext_lower}.gba"
  image_copy "$CONTROL_ROM" "ROMS/${CONTROL_ROM_NAME}"
fi

sync
if [[ "$filesystem" == exfat ]]; then
  sudo umount "$mount_dir"
  sudo losetup -d "$loop_dev"
  loop_dev=""
  rmdir "$mount_dir"
  trap - EXIT
fi

echo "Created $filesystem SD image: $out"
