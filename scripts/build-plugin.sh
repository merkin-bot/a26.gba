#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

docker pull devkitpro/devkitarm:latest >/dev/null
docker run --rm -v "$PWD:/work" -w /work devkitpro/devkitarm:latest \
  bash -lc 'cd archive-inputs/test-source/atari2600 && bash build.sh'
