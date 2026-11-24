#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/build-fixture.sh
fixture=source/atari2600/fixture.gba
export SDL_AUDIODRIVER=dummy
xvfb-run -a -s "-screen 0 1024x768x24" bash -c '
set -euo pipefail
cd "$1"
"/usr/games/mgba" -1 fixture.gba >/tmp/a26-menu.log 2>&1 &
pid=$!
trap "kill $pid 2>/dev/null || true" EXIT
sleep 4
xdotool keydown l; xdotool keydown r; sleep 0.25
xdotool keyup l; xdotool keyup r
sleep 1
xdotool key F12
' _ "$(dirname "$fixture")"
find "$(dirname "$fixture")" -maxdepth 1 -name 'fixture-*.png' -printf '%p\n'
