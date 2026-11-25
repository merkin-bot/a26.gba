#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

python3 scripts/setup-offline-workspace.py
source work/offline/devkitpro/devkitarm.sh

bash scripts/test-baseline.sh
cmake --build build/harness-cmake --target \
  supercard-model-test ezflash-sd-model-test ezflash-psram-model-test \
  ezflash-nor-model-test ezflash-mgba-trace-smoke ezflash-mgba-sd-smoke \
  supercard-mgba-runner ezflash-mgba-runner -j2
build/harness-cmake/supercard-model-test
build/harness-cmake/ezflash-sd-model-test
build/harness-cmake/ezflash-psram-model-test
build/harness-cmake/ezflash-nor-model-test
build/harness-cmake/ezflash-mgba-trace-smoke
build/harness-cmake/ezflash-mgba-sd-smoke

if [[ "${OFFLINE_SKIP_EXTENDED:-0}" != 1 ]]; then
  PATH="/usr/bin:$PATH" ASAN_OPTIONS=detect_stack_use_after_return=0 CC=gcc \
    bash harnesses/third_party/dssmm/superfw/scripts/verify.sh
  bash scripts/verify-ezflash-mgba.sh
else
  echo "Extended SuperFW/EZ suites skipped by OFFLINE_SKIP_EXTENDED=1."
fi

if [[ "${OFFLINE_SKIP_EXTENDED:-0}" == 1 ]]; then
  echo "Offline workspace core baseline passed; extended SuperFW/EZ suites skipped."
else
  echo "Offline workspace, SuperFW, and EZ harness baseline passed. No network access was used."
fi
