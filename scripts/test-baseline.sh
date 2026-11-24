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

if [[ ! -x /usr/games/mgba ]]; then
  echo "stock mGBA unavailable; install mgba-sdl" >&2
  exit 1
fi

plugin_launch="not-run-plugin-requires-PogoShell"
bash scripts/build-fixture.sh
bash scripts/run-mgba.sh source/atari2600/fixture.gba 3

echo "Plugin SHA-256: $plugin_hash"
echo "Stock mGBA plugin launch: skipped (PogoShell ABI absent)"
echo "Stock mGBA standalone fixture launch: survived 3 seconds"
