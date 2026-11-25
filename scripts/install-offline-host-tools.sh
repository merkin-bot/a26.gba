#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
packages="$PWD/vendor/offline/packages"
source_file=$(mktemp)
trap 'rm -f "$source_file"' EXIT
printf 'deb [trusted=yes] file:%s ./\n' "$packages" > "$source_file"

sudo apt-get \
  -o Dir::Etc::sourcelist="$source_file" \
  -o Dir::Etc::sourceparts=- \
  -o Acquire::Languages=none update
sudo apt-get \
  -o Dir::Etc::sourcelist="$source_file" \
  -o Dir::Etc::sourceparts=- \
  --no-install-recommends --yes install \
  build-essential cmake ninja-build pkg-config git python3 python3-pil \
  python3-unicorn lcov mtools dosfstools exfatprogs exfat-fuse fuse3 \
  stella mgba-sdl xvfb xdotool xauth

echo "Host tools installed from this repository's Ubuntu 24.04 offline package set."
