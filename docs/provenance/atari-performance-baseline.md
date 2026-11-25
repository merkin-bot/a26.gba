# Atari game-time baseline on the Chis mGBA model

**Date:** 2026-11-25 UTC  
**Purpose:** Record repeatable pre-optimization evidence for Atari game-time progress, video-frame production, input sequence, and GBA cycle budget.  
**Scope:** mGBA 0.10.2, SuperFW Build769 Chis source-shaped model, shared exFAT SD image. This is not physical GBA performance evidence.

## Exact setup

- Atari plugin: `source/atari2600/prototype.gba`, SHA-256 `cc868252fb113cd4160b8c6677d39cfe076056a4422f262b0975c99f5fb21de7`.
- Chis kernel: `harnesses/third_party/dssmm/superfw/build/chis/superfw.fw`, source commit `37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4`.
- Pristine SD image: `work/six-profile-exfat.img`, exFAT, 4 GiB, 32 KiB clusters, SHA-256 `b237c4c14f0091e88e4c5e535e7bdda7ce490f372de7c72db6fac9339b82d6c7`. Restore a new byte-for-byte copy before every runner invocation.
- Runner: `build/harness-cmake/supercard-mgba-runner`; source `harnesses/src/mgba_supercard_runner.c`; mGBA source commit `431935616c0e216d650b25b2ea0c0acccd359038`.
- Input events: `600:0x1,1200:0x1,1800:0x80,1900:0x1,2200-2210:0x1` over 3,200 GBA frames. The final 240x160 capture shows the test game screen with `OK?`; it is not a synchronized comparison to Stella.

Build the plugin, then run three fresh copies with the same command (the runner diagnostic observer is optional):

```sh
bash scripts/build-plugin.sh
cp --reflink=auto --sparse=always work/six-profile-exfat.img work/atari-baseline-sd.img
A26_SUPERCARD_BOARD=chis A26_TRACE_ATARI_GLOBALS=1 A26_TRACE_ATARI_INTERVAL=60 \
  build/harness-cmake/supercard-mgba-runner \
  harnesses/third_party/dssmm/superfw/build/chis/superfw.fw 3200 \
  work/atari-baseline-sd.img work/atari-baseline.ppm \
  '600:0x1,1200:0x1,1800:0x80,1900:0x1,2200-2210:0x1'
```

The optional Chis runner observer reads existing plugin globals from IWRAM using addresses verified from `prototype.elf` with `arm-none-eabi-nm`: `clk` `0x030001C4`, `clkcount` `0x03000044`, `tv_counter` `0x03000054`, Atari 6502 `program_counter` `0x030001D0`, beam line `0x03000F9C`, horizontal/vertical beam state `0x030008A8`/`0x030008B4`, and `pace_frame` `0x03002D90`. The observer only reads memory after `runFrame`; it does not write guest state. Recheck addresses after relinking the plugin.

## Results

Three cold runs each advanced exactly 3,200 GBA frames. All produced the same PPM SHA-256 `c2b27a39e9a32b404d937eb63682b3441c011885de2d4b42af5fbb4bb3271a11` and framebuffer FNV64 `A2DA827467B3C4A5`. Guest probes were identical. The Atari core first appeared at external frame 2,340 with `tv_counter=3`, `clk=58,193`; at frame 3,180 it reported `tv_counter=41`, `clk=802,414`. Across that 840-GBA-frame window, the runner observer counted 38 `tv_display` opportunities and 38 with rendering enabled; the final `pace_frame` sample was 1.

Across those 840 GBA frames, the Atari 6502 clock accumulator advanced by 744,221, averaging 19,585 Atari CPU clocks per completed TV frame. This is about 98.4% of the nominal 19,912 clocks for 262 NTSC lines at 76 CPU clocks per line. The current output rate was 38 rendered Atari frames per 840 GBA frames, or about 2.71 fps in the mGBA frame timeline. Thus the core advances close to native Atari game time per completed frame, but it cannot keep up with the GBA frame timeline. Using mGBA's 280,896 GBA cycles per frame, the observed workload is about 6.21 million GBA cycles per Atari frame; a 30 fps budget is about 559,000 cycles per frame, roughly an 11.1x reduction. These cycle-budget figures are derived from the pinned mGBA GBA model and global frame count; they are not measured ARM7 silicon timing.

The three host wall times were 4.32 s, 4.06 s, and 3.95 s for 3,200 mGBA frames. This variation reflects host execution and is not GBA speed evidence. The shared SD image passed `fsck.exfat -n` after runs and reported no SD load failures. The image and capture remain ignored under `work/`.

## Interpretation and limits

- The probe shows emulated Atari clock and frame-counter progression, not physical output cadence or real input latency.
- `pace_frame=1` at the recorded samples means the plugin did not skip those individual TIA display calls; it does not imply 30 or 60 rendered frames per second.
- The capture/hash only establishes repeatability for this cold-boot input scenario. It does not establish game correctness, audio, PAL behavior, broad ROM compatibility, or a synchronized Stella match.
- The observed gap prioritizes CPU/TIA scheduling and the interpreter/render path. Profile the built ARM code and measure a before/after change on this exact replay before accepting an optimization.

## Initial source inspection

`cpu.c:68-4616` is a large interpreter loop that dispatches one 6502 opcode through a 256-way `switch`, with opcode handlers updating global CPU state and `clkcount`. The linked `mainloop` occupies 21,556 bytes at `0x080005D8`; `decRead` is 1,364 bytes and `decWrite` is 1,572 bytes, also in the Game Pak image. Each loop advances `clk` and calls `do_screen(clkcount)`. `vmachine.c:339-395` advances beam/raster state from those instruction clocks; `raster.c:668-680` applies queued raster changes and renders a line; `display.c:183-219` refills 128 Direct Sound FIFO words and polls GBA Timer 1 on every `tv_display` call. The observer counted one skipped render over 41 calls from launch through the end of the run; most calls rendered. The timer threshold and audio work are candidates for profiling, not yet proven bottlenecks.
