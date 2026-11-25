#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "usage: $0 <retrobrews-clone> <game-stem> [output-dir]" >&2
  exit 2
fi

source_dir="$1"
game_stem="$2"
output_dir="${3:-work/test-roms}"
case "$game_stem" in
  ""|.*|*/*|*..*) echo "game stem must be a plain filename without path components" >&2; exit 2 ;;
esac
rom="$source_dir/$game_stem.bin"
metadata="$source_dir/$game_stem.txt"
if [[ ! -f "$rom" ]]; then
  echo "ROM not found: $rom" >&2
  exit 1
fi
if [[ ! -f "$metadata" ]]; then
  echo "per-game rights/usage metadata is missing: $metadata" >&2
  exit 1
fi

mkdir -p "$output_dir"
cp -- "$rom" "$output_dir/$game_stem.a26"
sha256sum "$output_dir/$game_stem.a26"
echo "Staged locally as $output_dir/$game_stem.a26; inspect $metadata before sharing or redistributing."
