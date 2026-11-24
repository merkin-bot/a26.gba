#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

rom="${1:-test-roms/flappy.a26}"
seconds="${2:-5}"
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-dummy}"
set +e
timeout "${seconds}s" xvfb-run -a /usr/games/mgba "$rom" >/tmp/a26-mgba.log 2>&1
rc=$?
set -e
if [[ $rc -ne 0 && $rc -ne 124 ]]; then
  cat /tmp/a26-mgba.log >&2
  exit "$rc"
fi
