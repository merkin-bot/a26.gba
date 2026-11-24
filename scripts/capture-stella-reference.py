#!/usr/bin/python3
"""Capture Stella TIA window and nearest-neighbor scale active frame to GBA 240x160."""
import os
import subprocess
import time
from pathlib import Path
from PIL import ImageGrab

root = Path(__file__).resolve().parents[1]
rom = root / "test-roms/flappy.a26"
out = root / "work/reference"
out.mkdir(parents=True, exist_ok=True)
env = dict(os.environ, SDL_AUDIODRIVER="dummy", XDG_RUNTIME_DIR="/tmp/stella-runtime")
proc = subprocess.Popen(["stella", "-fullscreen", "0", "-windowedpos", "0x0",
                         "-audio.enabled", "0", str(rom)],
                        env=env)
try:
    time.sleep(5)
    subprocess.run(["xdotool", "search", "--onlyvisible", "--name", "Stella"],
                   check=True, stdout=subprocess.DEVNULL)
    image = ImageGrab.grab().convert("RGB")
    (out / "stella-full.png").parent.mkdir(parents=True, exist_ok=True)
    image.save(out / "stella-full.png")
    # Stella's active TIA viewport is expected to be 160x192 before GBA scaling.
    width, height = image.size
    if width < 160 or height < 192:
        raise SystemExit("Stella viewport not large enough")
    # Center-crop the TIA area and scale by nearest-neighbor to GBA 240x160.
    left = max(0, (width - 160) // 2)
    top = max(0, (height - 192) // 2)
    tia = image.crop((left, top, left + 160, top + 192))
    gba = tia.resize((240, 160))
    gba.save(out / "stella-gba-240x160.png")
    print(out / "stella-gba-240x160.png")
finally:
    proc.terminate()
    try:
        proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()
