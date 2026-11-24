#!/usr/bin/env python3
"""Capture a stock-mGBA fixture frame and write a reproducible SHA-256."""
import hashlib
import os
import subprocess
import time
from pathlib import Path
from PIL import ImageGrab

root = Path(__file__).resolve().parents[1]
rom = root / "source/atari2600/fixture.gba"
if not rom.exists():
    raise SystemExit("fixture missing; run scripts/build-fixture.sh")

env = dict(os.environ, SDL_AUDIODRIVER="dummy", XDG_CONFIG_HOME="/tmp/a26-mgba-config")
proc = subprocess.Popen(["/usr/games/mgba", "-1", str(rom)], cwd=root, env=env)
try:
    time.sleep(4)
    subprocess.run(["xdotool", "key", "F12"], check=True)
    time.sleep(1)
    out = root / "work/fixture-frame.png"
    out.parent.mkdir(parents=True, exist_ok=True)
    image = ImageGrab.grab(bbox=(0, 0, 240, 160)).convert("RGB")
    image.save(out)
    digest = hashlib.sha256(out.read_bytes()).hexdigest()
    print(f"Frame: {out}")
    print(f"SHA-256: {digest}")
finally:
    proc.terminate()
    try:
        proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()
