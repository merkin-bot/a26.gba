#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

PLUGIN_GBA=source/atari2600/prototype.gba python3 scripts/build-card-fixture.py
seconds="${1:-5}"
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-dummy}"
timeout "${seconds}s" xvfb-run -a /usr/games/mgba work/card-fixture/a26-card-fixture.gba >/tmp/a26-card-mgba.log 2>&1 || rc=$?
rc="${rc:-0}"
if [[ "$rc" -ne 0 && "$rc" -ne 124 ]]; then
  cat /tmp/a26-card-mgba.log >&2
  exit "$rc"
fi
echo "PogoShell ABI cold-boot fixture survived $seconds seconds in stock mGBA"
