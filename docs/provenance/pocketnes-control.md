# PocketNES cross-card launch control

**Purpose:** Exercise PogoShell plugin handoff with a mature GBA NES emulator as a control, keeping its behavior separate from Atari emulation results.

## Sources and staged artifacts

- PocketNES: `https://github.com/Dwedit/PocketNES`, inspected commit `9c07f3d1d2a26b957769c5cde942060cc9ea6e64` (2025-07-19). Its `src/main.c` detects the PogoShell file head/tail at `0x0203FBFC` / `0x0203FBF8`, reads the argument block at `0x0203FC08`, and uses the appended file as the game ROM. `src/visoly.s` contains EZ-FLASH and SuperCard reset sequences. The checked-out revision has no top-level LICENSE file; source was built locally for this control and is not vendored or redistributed.
- NES homebrew catalog: `https://github.com/retrobrews/nes-games`, inspected commit `d20061bf9917e8bb8b947d4dba8c59372f5762a0` (2019-04-10). Selected `snailmaze.nes`, author Slydog Studios, metadata points to `slydogstudios.org`. The catalog README limits ROM distribution approval to the RetroBrew site/project. ROM remains local under ignored `work/`; do not commit or redistribute it.
- Local PocketNES GBA build SHA-256: `56489d3951c43081e5acbf2d508f7794dd2a37a17b5c27c00c72f5da2414256c` (`144,392` bytes). Local Snail Maze SHA-256: `1f6d866458dac2d1d857037f55339f2531447d0032608e8cf28bd34353366184` (`40,976` bytes). Both remain in ignored `work/`.

## Chis result

A fresh 64 MiB FAT image carried `SYSTEM/PLUG/nes.gba`, `ROMS/snailmaze.nes`, the matching Build769 Chis kernel, and the Atari plugin/ROM. `fsck.fat -n` passed and extracting each control file with `mtype` reproduced its staged SHA-256. `scripts/make-sd-image.sh` accepts the optional `CONTROL_PLUGIN_GBA`, `CONTROL_PLUGIN_EXT`, `CONTROL_ROM`, and `CONTROL_ROM_NAME` variables for this purpose.

The DSSMM Build769 `test-native-plugins-arm.py --pack ... --extension nes --game-bytes 40976` linked loader/FatFs/ABI fixture passed with the actual PocketNES GBA payload; it reported an 11,224-byte stack peak. Its game bytes are a generated same-size test seam, not the Snail Maze payload.

The full Chis mGBA kernel runner then received A taps to enter the SD browser and select Snail Maze. Over 1,500 GBA frames the model reported zero load failures, 716 SD read commands, 13 write commands, 1,432 synthetic blocks read, and 13 written. PC samples entered cartridge-mapped PocketNES code (`0x080001A0`, `0x0800290E`) after the kernel handoff. The final 240x160 capture showed PocketNES's Snail Maze title screen. This validates a software-model Chis launch path to that game screen; it does not qualify physical Chis hardware, real timing, play correctness, sound, or save persistence.

## EZ release-lane results

Fresh 64 MiB FAT images were built with the matching DE, Original OMEGA, or BLUE source kernel, the same PocketNES plugin, and Snail Maze ROM. Each pre-run image passed `fsck.fat -n`. All three runs used the same controlled events (`350-360:0x80,380-390:0x1,450-470:0x1,550-580:0x1`) and completed 2,500 GBA frames with zero synthetic SD-image failures:

- **OMEGA DE:** 803 blocks read, 216 written; S71 performed two erases and 12,672 word programs; framebuffer FNV64 `ABA1E9FDDE9F1C85`.
- **Original OMEGA:** 751 blocks read, 224 written; S71 performed one erase and 4,224 word programs; framebuffer FNV64 `ABA1E9FDDE9F1C85`.
- **BLUE:** 802 blocks read, 216 written; S98 performed one erase and 256 word programs; framebuffer FNV64 `3BDA64DE762CE6D0`.

The captures show PocketNES running with the Snail Maze content on all three source-shaped paths. The kernels reach different internal framebuffers/hashes, so these are per-profile launch observations, not synchronized image equivalence. The model keeps SD, PSRAM, S71, and S98 backing distinct. DE/OMEGA source uses S71 metadata; BLUE uses the separate S98 window and selects ROM page `0x0400` for plugin handoff. The adapter mirrors PSRAM into mGBA's `GBAMemory.rom` on source page commit because instruction fetch bypasses GBA data callbacks, and restores the kernel image on remap. This adapts mGBA's memory implementation; it does not assert physical EZ wiring.

The EZ results establish plugin/ROM handoff in these synthetic source-kernel paths only. They do not qualify physical cards, timing, gameplay, audio, save persistence across boots, or shell return. The physical PSRAM/NOR chips and their behavior remain `UNVERIFIED`.

## Menu reference

PocketNES uses L+R to open its in-game menu, directional navigation, A to activate an item, and B/R/L to close. Its familiar menu exposes autofire, controller, display, other settings, save/load state and SRAM management, sleep/link features, restart, and exit depending on build mode. The Atari plugin currently uses L+R, D-pad, A, and B, and has Controller, Display, Other Settings, Help, Resume, Restart, and Exit. It embeds the Goomba font resource, but its current screen is a simpler text list over a cleared display rather than PocketNES's animated/dimmed game background and larger option set. PocketNES's menu structure, visual style, font, and hotkeys are the product parity target; adapt the entries for Atari controllers, television mode, display, TIA audio, and Atari-specific actions. Compare behavior on synchronized, captured screens before claiming parity.
