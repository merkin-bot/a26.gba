#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

docker pull devkitpro/devkitarm:latest >/dev/null
docker run --rm -v "$PWD:/work" -w /work devkitpro/devkitarm:latest \
  bash -lc 'cd source/atari2600 && A26_FIXTURE=1 bash build.sh'
