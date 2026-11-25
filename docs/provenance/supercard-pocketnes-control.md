# SuperCard PocketNES control

**Reviewed:** 2026-11-25  
**Kernel source:** DSSMM SuperFW Build769, pinned commit `37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4`

## Purpose and image layout

The cross-profile exFAT storage result is documented in `harnesses/README.md` and `harnesses/cards.json`. The FAT32 runs below are earlier, separate PocketNES control experiments; the shared-image run does not inherit their launch/save observations.

PocketNES and RetroBrew Snail Maze provide a known-working PogoShell plugin/game control. PocketNES was built locally from `work/PocketNES` at commit `9c07f3d1d2a26b957769c5cde942060cc9ea6e64`; its GBA SHA-256 is `56489d3951c43081e5acbf2d508f7794dd2a37a17b5c27c00c72f5da2414256c`. Snail Maze was staged locally from RetroBrew NES catalog commit `d20061bf9917e8bb8b947d4dba8c59372f5762a0`; its SHA-256 is `1f6d866458dac2d1d857037f55339f2531447d0032608e8cf28bd34353366184`. Both remain ignored under `work/`; do not redistribute the ROM.

Each fresh 64 MiB FAT32 profile image was made with `scripts/make-sd-image.sh`. The matching Build769 kernel is a separate runner input. The expected PogoShell paths were verified with `mdir`:

- `SYSTEM/PLUG/nes.gba` contains the PocketNES plugin.
- `SYSTEM/SAVER/` exists and is initially empty; this is the kernel's save directory.
- The matching SD, Lite, or Chis kernel is passed directly to `supercard-mgba-runner`; it is not stored on the SD image.
- `ROMS/snailmaze.nes` contains the NES control ROM. The image builder also stages the Atari plugin/test ROM under their canonical paths.

## Cross-board launch observation

The same frame-indexed key taps were used for each matching kernel and fresh image:

```sh
events='600:0x1,1200:0x1,1800:0x80,1900:0x1'
A26_SUPERCARD_BOARD=lite build/harness-cmake/supercard-mgba-runner \
  harnesses/third_party/dssmm/superfw/build/lite/superfw.fw 2200 \
  work/sc-lite-pocketnes-clean.img work/sc-lite-pocketnes-clean-2200.ppm "$events"
```

Replace `lite` with `sd` or `chis` in the environment, kernel path, image, and output path. On all three profiles the runner reached cartridge-mapped PocketNES code (first reported PC in cartridge: SD frame 2058, Lite frame 2034, Chis frame 2034). At frame 2200 each reported 716 SD read commands, 13 write commands/blocks, 1,432 model blocks loaded, zero load failures, and framebuffer FNV64 `C8F4325206720D24`. The three 240x160 PPM captures are byte-identical (SHA-256 `8595aa48e864a9f3f83fbb7b768b953ca791b7f023049ae37d65da1266ce2fb4`) and show the PocketNES/Slydog startup splash. Each initial image passed `fsck.fat -n` after the run.

The selected-frame evidence establishes a matching-kernel, source-shaped mGBA launch path through the PocketNES startup screen for SD, Lite, and Chis. It does not yet establish the Snail Maze title/game screen, sustained gameplay, real card behavior, GBA timing, or shell return. Earlier captures at 2,500 frames ended back in the SuperFW browser; do not use those late frames as launch-failure evidence because the plugin had executed earlier.

## Save and return observations

Fresh SD, Lite, and Chis images with no `SYSTEM/KERNELS` path or theme-style payload were each run a second time for 3,000 GBA frames with the same input events (`2350:0x8,2450:0x1`). Each created `SYSTEM/SAVER/snailmaze.sav` (131,072 bytes). Extracted saves from all profiles have the same SHA-256, `b5a41c3758763bbec72769fab4a2533bf2db0b6312d93d25a695f9e4b9e02260`. `fsck.fat -n` passed after each run. Their final 240x160 captures are byte-identical (SHA-256 `5bb42bf22a46219a8777518c103dc924b03faf2f75786899b0fcdd30ce4c79dc`); the final PC on each is in the matching Build769 kernel's EWRAM (`0x027DD7xx` on SD/Lite; `0x027DFC68` on Chis), consistent with execution having returned from cartridge code to the kernel UI. The frame-2200 capture remains the only confirmed image of the PocketNES startup splash. No capture yet establishes the Snail Maze title/game screen or sustained play, and the synthetic save result does not prove correctness, physical power-cycle persistence, or physical-card behavior.

The mGBA SuperCard control path now covers kernel boot, plugin and ROM handoff, matching per-profile SD/block adapter, save-file creation/readback, and return to kernel code for SD/Lite/Chis. The EZ DE/Original OMEGA/BLUE control runs are separately recorded in `PROGRESS.md` and `harnesses/README.md`. This closes the six-profile software harness gate only to the documented synthetic model boundary. Real card/kernel timing, electrical behavior, and persistence remain `UNVERIFIED`.

Remaining control work: identify a reproducible input/timing sequence that displays the Snail Maze title/game screen. Physical card qualification is unavailable in this workspace. All mGBA results remain software-model evidence, not physical behavior or GBA performance evidence.

All results above are mGBA and synthetic storage-model evidence. Physical board mappings, SD timing, persistence, NOR/SRAM behavior, and compatibility remain `UNVERIFIED`.
