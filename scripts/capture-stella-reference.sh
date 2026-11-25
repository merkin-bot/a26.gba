#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "usage: $0 <atari-rom> <output-240x160.png> [horizontal-crop-pixels-per-side]" >&2
  exit 2
fi

rom_path=$(realpath "$1")
output_path=$(realpath -m "$2")
horizontal_crop=${3:-12}
if [[ ! "$horizontal_crop" =~ ^[0-9]+$ ]] || (( horizontal_crop >= 80 )); then
  echo "horizontal crop must be an integer from 0 through 79 pixels per side" >&2
  exit 2
fi
tmp_dir=$(mktemp -d)
snapshot_dir="$tmp_dir/snapshots"
mkdir -p "$snapshot_dir" "$tmp_dir/runtime"
chmod 700 "$tmp_dir/runtime"
trap 'rm -rf "$tmp_dir"' EXIT

if ! command -v stella >/dev/null || ! command -v xvfb-run >/dev/null || \
   ! command -v xdotool >/dev/null || ! command -v python3 >/dev/null; then
  echo "Stella, Xvfb, xdotool, and Python 3 are required; see WORKSPACE.md" >&2
  exit 1
fi

if ! /usr/bin/python3 -c 'from PIL import Image' >/dev/null 2>&1; then
  echo "python3-pil is required; see WORKSPACE.md" >&2
  exit 1
fi

mkdir -p "$(dirname "$output_path")"
snapshot_stem=$(basename "$rom_path")
snapshot_stem=${snapshot_stem%.*}

xvfb-run -a --server-args='-screen 0 640x480x24' \
  env SDL_AUDIODRIVER=dummy XDG_RUNTIME_DIR="$tmp_dir/runtime" \
  sh -c '
    stella -video software -vsync 0 -audio.enabled 0 -tia.zoom 1 \
      -sssingle 1 -ss1x 1 -snapname rom -snapsavedir "$1" "$2" \
      >"$3/stella.log" 2>&1 &
    stella_pid=$!
    trap "kill \$stella_pid 2>/dev/null || true" EXIT
    sleep 2
    xdotool key F12
    attempt=0
    while [ ! -s "$1/$4.png" ] && [ "$attempt" -lt 50 ]; do
      sleep 0.1
      attempt=$((attempt + 1))
    done
    test -s "$1/$4.png"
  ' sh "$snapshot_dir" "$rom_path" "$tmp_dir" "$snapshot_stem"

/usr/bin/python3 - "$snapshot_dir/$snapshot_stem.png" "$output_path" "$horizontal_crop" <<'PY'
import sys
from PIL import Image

source_path, output_path, crop_arg = sys.argv[1:]
with Image.open(source_path) as source:
    source = source.convert("RGB")
    if source.width % 160:
        raise SystemExit(f"Stella snapshot width {source.width} is not an integer multiple of 160")
    native = source.resize((160, source.height), Image.Resampling.NEAREST)
    if native.height < 192:
        raise SystemExit(f"Stella snapshot has only {native.height} scanlines; expected at least 192")
    top = (native.height - 192) // 2
    playfield = native.crop((0, top, 160, top + 192))
    if crop_x := int(crop_arg):
        playfield = playfield.crop((crop_x, 0, 160 - crop_x, 192))
    scaled = playfield.resize((240, 160), Image.Resampling.NEAREST)
    scaled.save(output_path, format="PNG", optimize=False)
print(f"Stella snapshot {source.width}x{source.height} -> 160x192 TIA viewport, {crop_arg}px side crop -> 240x160: {output_path}")
PY
