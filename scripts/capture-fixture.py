#!/usr/bin/env python3
"""Capture a stock-mGBA fixture frame and write a reproducible SHA-256."""
import hashlib
import os
import subprocess
import time
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[1]
rom = root / "source/atari2600/fixture.gba"
if not rom.exists():
    raise SystemExit("fixture missing; run scripts/build-fixture.sh")

out_dir = root / "work"
out_dir.mkdir(parents=True, exist_ok=True)
run_rom = out_dir / "fixture-run.gba"
run_rom.write_bytes(rom.read_bytes())
env = dict(os.environ, SDL_AUDIODRIVER="dummy", XDG_CONFIG_HOME="/tmp/a26-mgba-config")
proc = subprocess.Popen(["/usr/games/mgba", "-1", str(run_rom)], cwd=out_dir, env=env)
try:
    time.sleep(4)
    subprocess.run(["xdotool", "key", "F12"], check=True)
    time.sleep(1)
    screenshots = sorted(out_dir.glob("fixture-run-*.png"), key=lambda path: path.stat().st_mtime)
    if not screenshots:
        raise SystemExit("mGBA did not save an F12 emulator screenshot")
    out = out_dir / "fixture-frame.png"
    image = Image.open(screenshots[-1]).convert("RGB")
    if image.size != (240, 160):
        raise SystemExit(f"unexpected mGBA frame size: {image.size}")
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
