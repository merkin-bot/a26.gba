# SuperCard PocketNES control

**Reviewed:** 2026-11-25  
**Kernel source:** DSSMM SuperFW Build769, pinned commit `37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4`

## Purpose and image layout

PocketNES and RetroBrew Snail Maze provide a known-working PogoShell plugin/game control. PocketNES was built locally from `work/PocketNES` at commit `9c07f3d1d2a26b957769c5cde942060cc9ea6e64`; its GBA SHA-256 is `56489d3951c43081e5acbf2d508f7794dd2a37a17b5c27c00c72f5da2414256c`. Snail Maze was staged locally from RetroBrew NES catalog commit `d20061bf9917e8bb8b947d4dba8c59372f5762a0`; its SHA-256 is `1f6d866458dac2d1d857037f55339f2531447d0032608e8cf28bd34353366184`. Both remain ignored under `work/`; do not redistribute the ROM.

Each fresh 64 MiB FAT32 profile image was made with `scripts/make-sd-image.sh` and the matching Build769 kernel. The expected PogoShell paths were verified with `mdir`:

- `SYSTEM/PLUG/nes.gba` contains the PocketNES plugin.
- `SYSTEM/SAVER/` exists and is initially empty; this is the kernel's save directory.
- `SYSTEM/KERNELS/KERNEL.GBA` contains the exact matching SD, Lite, or Chis kernel.
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

## Save directory observation and remaining work

The first Lite boot left `SYSTEM/SAVER` empty. A second Lite boot from the same image created `SYSTEM/SAVER/snailmaze.sav` (131,072 bytes; extracted SHA-256 `b5a41c3758763bbec72769fab4a2533bf2db0b6312d93d25a695f9e4b9e02260`). The post-run Lite image passed `fsck.fat -n`. This is evidence that the source-shaped Lite path creates the expected save file and that it remains in the raw image across runner invocations. It does not prove the save contents are valid, survive an actual card power cycle, or work on SD/Chis. Repeat boot/save extraction on fresh SD and Chis images, compare save behavior, then find an input that advances beyond the PocketNES splash and verify the Snail Maze title screen and shell return.

All results above are mGBA and synthetic storage-model evidence. Physical board mappings, SD timing, persistence, NOR/SRAM behavior, and compatibility remain `UNVERIFIED`.
