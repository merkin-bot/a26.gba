#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

bash scripts/build-plugin.sh
python3 scripts/test-pogo-abi.py

plugin="archive-inputs/test-source/atari2600/prototype.gba"
plugin_hash=$(sha256sum "$plugin" | awk '{print $1}')
archived_hash=$(sha256sum archive-inputs/a26.gba | awk '{print $1}')
if [[ "$plugin_hash" != "$archived_hash" ]]; then
  echo "Plugin hash mismatch: build=$plugin_hash archive=$archived_hash" >&2
  exit 1
fi

if [[ ! -x /usr/games/mgba ]]; then
  echo "stock mGBA unavailable; install mgba-sdl" >&2
  exit 1
fi

plugin_launch="not-run-plugin-requires-PogoShell"
bash scripts/build-fixture.sh
bash scripts/run-mgba.sh archive-inputs/test-source/atari2600/fixture.gba 3

echo "Plugin SHA-256: $plugin_hash"
echo "Stock mGBA plugin launch: skipped (PogoShell ABI absent)"
echo "Stock mGBA standalone fixture launch: survived 3 seconds"
