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

## First placement candidate: interpreter loop in IWRAM

**Candidate:** `mainloop()` is placed in IWRAM and three TIA raster event queues (`pl_change`, `pf_change`, `unified`) are placed in EWRAM. Plugin SHA-256 `fa851c1de2d7b3700a28ae29ad94bd36c98e6bd866735f8f98f6c541a86ab724`. This is a placement-only optimization; it does not alter Atari emulation logic.

The runner's optional `A26_STOP_ATARI_TV_COUNTER=41` stops at an Atari TV count boundary. The before image used the baseline plugin and its original IWRAM offsets; the after image used the candidate plugin and its relinked offsets. Both used the same input sequence and pristine shared exFAT image. Both reached `tv_counter=41`, rendered 40 display frames, and produced byte-identical PPM captures (SHA-256 `c11fdf92de4b7fb70c1e23f6635b4c7527b2bcb995410443663c9126d8b20080`). The baseline required 3,179 external mGBA frames; the candidate required 3,083, about 3.0% fewer. Atari clock accumulators at the boundary were 800,745 and 801,494 respectively. WAITCNT remained `0x4000` (ROM0 non-sequential 4 and sequential 2 halfword cycles) in both runs. Both reported zero SD load failures.

This is a small diagnostic improvement, far below the performance target. It compares work to the same Atari TV counter, but not a cycle-accurate ARM7 timing measurement or physical GBA result. The framebuffer match is the available output check; no claim of full game-state equivalence follows from matching output alone. The next candidate is moving the frequently called `do_screen()` routine into IWRAM and repeating the same boundary comparison.

## Second placement candidate: TIA screen routine in IWRAM

**Candidate:** `do_screen()` now executes from IWRAM along with `mainloop()`. Plugin SHA-256 `401b61c21e93dcc813179a81a0e3a16cbc821a74d2bf73a41464be412c8b3cad`; linked `.iwram` is 21,968 bytes. The same input replay and Chis runner stopped at `tv_counter=41` for the first placement candidate and this build. Both rendered 40 frames and produced byte-identical PPM captures (SHA-256 `c11fdf92de4b7fb70c1e23f6635b4c7527b2bcb995410443663c9126d8b20080`). The first candidate reached the boundary in 3,083 mGBA frames at `clk=801,494`; this candidate took 3,036 frames at `clk=800,241`, about 1.5% fewer frames. Active WAITCNT remained `0x4000` (ROM0 4/2 halfword cycles), and both images passed `fsck.exfat -n`.

Three cold replays per candidate confirm a small, repeatable change in this mGBA model. The prior build stopped at 3,083, 3,081, and 3,079 frames (mean 3,081); the `do_screen()` build stopped at 3,036, 3,038, and 3,038 (mean 3,037), about 1.4% fewer frames. All six captures had the same SHA-256 above, and all reported zero SD load failures. Atari accumulator readings at the stop varied by several thousand clocks around 801k, so the boundary timing is not exact cycle-equivalence. SD command counts also varied substantially between cold replays (the `do_screen()` build ranged from 32,544 to 54,192 command writes and 6,877 to 144,946 data writes), and are retained as a caveat about changed guest progress/timing. This remains an mGBA-only signal, far below the PocketNES-like target; physical timing is unmeasured. Continue by inspecting hot memory accessors and rendering cost.

## Interpretation and limits

- The probe shows emulated Atari clock and frame-counter progression, not physical output cadence or real input latency.
- `pace_frame=1` at the recorded samples means the plugin did not skip those individual TIA display calls; it does not imply 30 or 60 rendered frames per second.
- The capture/hash only establishes repeatability for this cold-boot input scenario. It does not establish game correctness, audio, PAL behavior, broad ROM compatibility, or a synchronized Stella match.
- The observed gap prioritizes CPU/TIA scheduling and the interpreter/render path. Profile the built ARM code and measure a before/after change on this exact replay before accepting an optimization.

## Initial source inspection

`cpu.c:68-4616` is a large interpreter loop that dispatches one 6502 opcode through a 256-way `switch`, with opcode handlers updating global CPU state and `clkcount`. The linked `mainloop` occupies 21,556 bytes at `0x080005D8`; `decRead` is 1,364 bytes and `decWrite` is 1,572 bytes, also in the Game Pak image. Each loop advances `clk` and calls `do_screen(clkcount)`. `vmachine.c:339-395` advances beam/raster state from those instruction clocks; `raster.c:668-680` applies queued raster changes and renders a line; `display.c:183-219` refills 128 Direct Sound FIFO words and polls GBA Timer 1 on every `tv_display` call. The observer counted one skipped render over 41 calls from launch through the end of the run; most calls rendered. The timer threshold and audio work are candidates for profiling, not yet proven bottlenecks.
