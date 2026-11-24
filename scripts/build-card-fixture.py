#!/usr/bin/python3
"""Build a stock-mGBA PogoShell launch fixture for EZ/SuperCard-style ABI."""
import argparse
import json
import os
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ROM_BASE = 0x08000000
ABI = 0x0203FBF8

parser = argparse.ArgumentParser()
parser.add_argument("--out", type=Path, default=ROOT / "work/card-fixture")
args = parser.parse_args()
out = args.out.resolve()
out.mkdir(parents=True, exist_ok=True)

plugin_path = Path(os.environ.get("PLUGIN_GBA", ROOT / "source/atari2600/prototype.gba"))
plugin = bytearray(plugin_path.read_bytes())
rom = (ROOT / "test-roms/flappy.a26").read_bytes()
if len(rom) not in (2048, 4096, 8192, 16384):
    raise SystemExit("unsupported Atari ROM size")

seed_offset = (len(plugin) + 3) & ~3
rom_offset = seed_offset + 512
first = struct.unpack_from("<I", plugin, 0)[0]
entry_target = ROM_BASE + 8 + ((first & 0xFFFFFF) << 2)
seed_branch = seed_offset - 8

seed = f""".syntax unified
.arm
.global _start
_start:
 ldr r0,=0x0203fbf8
 mov r1,#4096
 str r1,[r0]
 ldr r1,=0x08000000+{rom_offset}
 str r1,[r0,#4]
 ldr r1,=0xfab0babe
 str r1,[r0,#8]
 mov r1,#2
 str r1,[r0,#12]
 ldr r1,=0x0203fc08
 mov r2,#0
 str r2,[r1]
 ldr pc,=0x{entry_target:x}
.pool
"""
seed_src = out / "card-seed.S"
seed_src.write_text(seed)
import os
import subprocess
docker = ["docker", "run", "--rm", "-v", f"{ROOT}:/work", "-w", "/work",
          "devkitpro/devkitarm:latest"]
subprocess.run(docker + ["bash", "-lc",
    f"/opt/devkitpro/devkitARM/bin/arm-none-eabi-as -mcpu=arm7tdmi work/card-fixture/card-seed.S -o work/card-fixture/card-seed.o && "
    "/opt/devkitpro/devkitARM/bin/arm-none-eabi-objcopy -O binary work/card-fixture/card-seed.o work/card-fixture/card-seed.bin"],
    check=True)
seed_bin = (out / "card-seed.bin").read_bytes()
if len(seed_bin) > 512:
    raise SystemExit("PogoShell seed exceeds fixture padding")

plugin.extend(bytes(seed_offset - len(plugin)))
plugin.extend(seed_bin + bytes(512 - len(seed_bin)))
plugin.extend(rom + bytes(max(0, 4096 - len(rom))))
struct.pack_into("<I", plugin, 0, 0xEA000000 | ((seed_offset // 4) - 2))

fixture = out / "a26-card-fixture.gba"
fixture.write_bytes(plugin)
(out / "fixture.json").write_text(json.dumps({
    "purpose": "PogoShell ABI cold boot for stock mGBA",
    "plugin_sha256": __import__("hashlib").sha256(plugin_path.read_bytes()).hexdigest(),
    "rom_sha256": __import__("hashlib").sha256(rom).hexdigest(),
    "rom_offset": rom_offset,
    "limits": "Portable ABI and card launch state only; SuperCard/EZ card I/O not emulated."
}, indent=2) + "\n")
print(fixture)
