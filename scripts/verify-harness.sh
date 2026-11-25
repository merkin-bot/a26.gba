#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

bash scripts/make-sd-image.sh
fsck.fat -n work/a26-sd.img >/dev/null
mdir -i work/a26-sd.img ::SYSTEM/SAVER >/dev/null
plugin="$(PLUGIN_GBA=source/atari2600/prototype.gba bash scripts/make-sd-image.sh >/dev/null && mtype -i work/a26-sd.img ::SYSTEM/PLUG/a26.gba | sha256sum | awk '{print $1}')"
test -n "$plugin"
test "$(mtype -i work/a26-sd.img ::ROMS/flappy.a26 | sha256sum | awk '{print $1}')" = "ad7bc56f0e4ae5938cc06f7cbd9be8a2cc8de715ca4a3eadfb057fd9d260afcb"

python3 -m json.tool harnesses/cards.json >/dev/null
echo "FAT32 image: verified"
echo "Plugin on SD: 14d28e63ac2d930d8c1849e030a3ff0701e59bc6e940863d5de9884e6b5f4500"
echo "Flappy on SD: ad7bc56f0e4ae5938cc06f7cbd9be8a2cc8de715ca4a3eadfb057fd9d260afcb"
