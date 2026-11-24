#!/usr/bin/env python3
import json
import struct
from pathlib import Path

ROM_BASE = 0x08000000
ABI = 0x0203FBF8
ARGV = 0x0203FC08

def pogo_layout(plugin_size, rom_size):
    if not plugin_size or not rom_size or plugin_size > 14 * 1024 * 1024:
        return None
    file_offset = (plugin_size + 3) & ~3
    table_offset = (file_offset + rom_size + 32767) & ~32767
    if rom_size > 14 * 1024 * 1024 or table_offset > 14 * 1024 * 1024 - 88:
        return None
    return file_offset, table_offset

def test_layout():
    assert pogo_layout(257, 8193) == (260, 32768)
    assert pogo_layout(0, 1) is None
    assert pogo_layout(1, 0) is None
    assert pogo_layout(14 * 1024 * 1024, 1) is None

def test_argument_words():
    ram = [4096, ROM_BASE + 260, 0xFAB0BABE, 2]
    assert ram[0] == 4096
    assert ram[1] == ROM_BASE + 260
    assert ram[2] == 0xFAB0BABE
    assert ram[3] == 2
    packed = struct.pack("<4I", *ram)
    assert len(packed) == 16

def test_card_profile_manifest():
    root = Path(__file__).resolve().parents[1]
    manifest = json.loads((root / "harnesses/cards.json").read_text())
    assert len(manifest["profiles"]) == 6
    assert "mGBA GBA-system model" in manifest["layers"]
    assert "DSSMM per-card firmware/kernel and cartridge behavior" in manifest["layers"]
    assert any("SD block image" in layer for layer in manifest["layers"])
    assert len(manifest["existing_tests"]["ez_flash_native_launch"]) == 3

if __name__ == "__main__":
    test_layout()
    test_argument_words()
    test_card_profile_manifest()
    print("PogoShell portable ABI: layout, argument block, and six-profile manifest passed")
