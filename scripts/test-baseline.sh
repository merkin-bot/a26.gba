#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

bash scripts/build-plugin.sh
python3 scripts/test-pogo-abi.py
bash scripts/verify-harness.sh
mkdir -p build/harness
cc -std=c11 -Wall -Wextra -Werror -pedantic -Iharnesses/include \
  harnesses/src/supercard_model.c harnesses/tests/supercard_model_test.c \
  -o build/harness/supercard-model-test
build/harness/supercard-model-test

plugin="source/atari2600/prototype.gba"
plugin_hash=$(sha256sum "$plugin" | awk '{print $1}')

bash scripts/build-fixture.sh
cmake -S harnesses -B build/harness-cmake -G Ninja
cmake --build build/harness-cmake --target mgba-core-smoke -j2
build/harness-cmake/mgba-core-smoke
bash scripts/run-mgba.sh source/atari2600/fixture.gba 3
bash scripts/run-card-fixture.sh 3

echo "Plugin SHA-256: $plugin_hash"
echo "mGBA GBA core bus smoke and standalone/card fixture boots: passed"
echo "Plugin launch: skipped (PogoShell ABI absent)"
