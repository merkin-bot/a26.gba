#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
export DEVKITARM="${DEVKITARM:-$DEVKITPRO/devkitARM}"
export PATH="$DEVKITARM/bin:$DEVKITPRO/tools/bin:$PATH"

if ! command -v arm-none-eabi-gcc >/dev/null || ! command -v gbafix >/dev/null; then
  echo "devkitARM and devkitPro general-tools are required; see WORKSPACE.md" >&2
  exit 1
fi

cd source/atari2600
A26_FIXTURE=1 bash build.sh
