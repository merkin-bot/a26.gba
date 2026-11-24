#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p /tmp/stella-runtime
xvfb-run -a -s "-screen 0 1024x768x24" /usr/bin/python3 scripts/capture-stella-reference.py
