#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

rom="${1:-archive-inputs/flappy.a26}"
seconds="${2:-5}"
timeout "${seconds}s" xvfb-run -a /usr/games/mgba "$rom"
