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
    # Stella letterboxes its active TIA image inside the Xvfb desktop. Find the
    # rendered bounds instead of cropping the desktop by its aspect ratio.
    width, height = image.size
    if width < 240 or height < 160:
        raise SystemExit("Stella display is smaller than GBA resolution")
    mask = image.convert("L").point(lambda value: 255 if value > 8 else 0)
    bounds = mask.getbbox()
    if bounds is None:
        raise SystemExit("Stella frame is blank")
    tia = image.crop(bounds)
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
