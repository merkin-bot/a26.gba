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

## Third placement candidate: decoded data reads in IWRAM

`decRead()` was moved from cartridge ROM to IWRAM (plugin SHA-256 `5d60fbe81de5d066edd3166c6c9ce3f15a3649dd507636cac2a11f9e523a3bc1`). This adds about 1,488 bytes to `.iwram`; it does not change emulator logic. Three fixed-TV-counter replays on the prior build took 3,036, 3,038, and 3,036 mGBA frames (mean 3,036.7); the new build took 3,023, 3,027, and 3,025 (mean 3,025), about 0.38% fewer. Captures stayed byte-identical at SHA-256 `c11fdf92de4b7fb70c1e23f6635b4c7527b2bcb995410443663c9126d8b20080`, rendered counts were 40, WAITCNT stayed 0x4000, and SD load failures were zero.

The linked `.iwram`, `.bss`, and `.data` end at `0x03007A3C`; initial IWRAM stack top is `0x03007F00`, leaving 1,220 bytes before the change. A new runner diagnostic samples the GBA SP after every GBA frame once Atari code is active. Both builds reached a lowest sampled SP of `0x03007DC8`, leaving 908 bytes above static data in this replay. This is sampled stack use, not a proof of the absolute worst-case IRQ/call-chain stack requirement. Given the small gain and remaining uncertainty, retain the placement only provisionally and revisit if later code needs IWRAM. Physical hardware timing remains unmeasured.

## Rejected ARM-mode and EWRAM loop experiments

The baseline `mainloop()` is Thumb code in IWRAM. Applying GCC's per-function `target("arm")` while keeping the loop in IWRAM failed to link: `.data` was placed at `0x03009274`, outside the 32 KiB IWRAM region. Moving that ARM-mode loop to EWRAM linked as plugin SHA-256 `20df99c5a99dc68c630abf98459330ff8f789b7aa14cf7a804763a80dd671f10`; its `.ewram` code was 27,856 bytes. Two 3,200-frame Chis runs each reached only 35 Atari display calls (34–35 rendered) and produced the same different framebuffer hash `cbce2ec7429bd3c67e26a244909d3708f46496bec6161b4e63c4cbf8209c69f2`; the Thumb/IWRAM comparison reached 41 calls and hash `3F6510F57242D0B5`. Active WAITCNT remained `0x4000`, and no SD load failures occurred. The ARM/EWRAM candidate is rejected and its source change was reverted.

Source audit at pinned upstream revisions found no faster loop to transplant from the classic GBA project: `MikeDX/2600_gba` commit `2eb18dd8a5b832c939efe510b4e493ad4cf25437` has the same switch-driven C loop. `PocketStella` commit `1e299b9e6c3197f6ad63a5a06eaf4db1c981bc43` builds a separate ARM 6502 assembly source which is absent from that checkout, so its checked-in object is not a suitable source basis. `StellaDS` commit `50dbad2f068364f6c3892f379ac4212803784607` is GPL-2 and targets DS ARM9; its `M6502Low` uses a local PC and direct page-fetch fast paths, worth studying as a future architectural port but not a drop-in GBA implementation. These references are listed in `reference/repositories.txt`; no code was copied in these experiments.

## Compiler level: `-O3` versus `-O2`

The build script now selects `-O3` by default and permits `A26_OPT_LEVEL=2` for exact comparison; other accepted levels are 0, 1, and `s`. The O2 plugin was `5d60fbe81de5d066edd3166c6c9ce3f15a3649dd507636cac2a11f9e523a3bc1`; the O3 plugin was `dc487cf29dce0785cedf1ac0f892d84f29e8e48ca3baf1ff254bace98ac744f3`. Three fixed-TV-counter replays averaged 3,025 mGBA frames at O2 and 2,989 at O3 (~1.2% fewer). All six captures matched byte-for-byte (PPM SHA-256 `c11fdf92de4b7fb70c1e23f6635b4c7527b2bcb995410443663c9126d8b20080`); all runs reached TV counter 41, rendered 40 frames, and had zero SD load failures. The lowest sampled SP was `0x03007DC8` at O2 and `0x03007DD0` at O3. O3 increases linked IWRAM by 24 bytes, leaving the same safe margin measured above. This is a reproducible but modest mGBA-only gain, not evidence of physical GBA performance.

## Interpretation and limits

- The probe shows emulated Atari clock and frame-counter progression, not physical output cadence or real input latency.
- `pace_frame=1` at the recorded samples means the plugin did not skip those individual TIA display calls; it does not imply 30 or 60 rendered frames per second.
- The capture/hash only establishes repeatability for this cold-boot input scenario. It does not establish game correctness, audio, PAL behavior, broad ROM compatibility, or a synchronized Stella match.
- The observed gap prioritizes CPU/TIA scheduling and the interpreter/render path. Profile the built ARM code and measure a before/after change on this exact replay before accepting an optimization.

## Initial source inspection

`cpu.c:68-4616` is a large interpreter loop that dispatches one 6502 opcode through a 256-way `switch`, with opcode handlers updating global CPU state and `clkcount`. The linked `mainloop` occupies 21,556 bytes at `0x080005D8`; `decRead` is 1,364 bytes and `decWrite` is 1,572 bytes, also in the Game Pak image. Each loop advances `clk` and calls `do_screen(clkcount)`. `vmachine.c:339-395` advances beam/raster state from those instruction clocks; `raster.c:668-680` applies queued raster changes and renders a line; `display.c:183-219` refills 128 Direct Sound FIFO words and polls GBA Timer 1 on every `tv_display` call. The observer counted one skipped render over 41 calls from launch through the end of the run; most calls rendered. The timer threshold and audio work are candidates for profiling, not yet proven bottlenecks.

## Re-established boundary after Direct Sound changes

The Direct Sound change altered linked IWRAM globals, so the runner's original default observer offsets became stale. An attempted replay using those offsets read unrelated memory and was discarded. Current offsets were verified against `arm-none-eabi-nm -n source/atari2600/prototype.elf` and are now the runner defaults: `program_counter=0x03005D90`, `clk=0x03005D84`, `clkcount=0x03005BFC`, `tv_counter=0x03005C0C`, `ebeamy=0x0300679C`, `hbeam_state=0x03006468`, `vbeam_state=0x03006474`, and `pace_frame=0x03007A50`.

With the Direct Sound plugin SHA-256 `ab91d38db0ad10c6c93ebdf930018815e93559d088a8a8679eef729c103d4964`, three fresh fixed-TV-counter runs all stopped at Atari TV counter 41 after 2,995 mGBA frames with `clk=805693`, 41 rendered display calls, minimum sampled SP `0x03007DD0`, and PPM SHA-256 `c11fdf92de4b7fb70c1e23f6635b4c7527b2bcb995410443663c9126d8b20080`. This supersedes the old observer offsets for the current linked image; any plugin relink must recheck them.

A branch-flattened `do_screen()` rewrite was tested in three runs and reverted: it had the same whole-frame boundary, state, captures, and stack observations. Since the boundary metric resolves only whole external mGBA frames, this was inconclusive about sub-frame changes and is not retained as an optimization. Callgrind host profile remains diagnostic support for the next candidate, not GBA timing evidence.

## mGBA host Callgrind profile

The same 3,200-frame Chis replay was run under Valgrind Callgrind 3.22.0 (with no Atari stop counter, to retain the normal runner workload). The profile recorded 40,269,249,886 host instruction events. Inclusive attribution placed 99.94% in `_GBACoreRunFrame`, 99.85% in mGBA's `ARMRunLoop`; within that interpreter path, `GBAProcessEvents` accounted for 44.27%, `_ThumbInstructionSTRH1` 12.40%, and `_ThumbInstructionBNE` 4.97% of total host instruction events. The rest of the displayed instruction handlers were each below these values.

This explains where the desktop harness spends host work while emulating the GBA; it does not measure GBA ARM7 cycles or attribute those guest instructions to Atari plugin routines. In particular, do not optimize the mGBA core or storage model to make Atari appear faster. Source-level Atari candidates still require the same fixed Atari boundary and capture checks; GBA hardware timing requires hardware evidence.

## Precise raster fast-path comparison

On 2026-11-26, tested the token-limit commit's `mainloop()` shortcut that updates `ebeamx` directly when both beam states are `DRAWSTATE` and the instruction remains within the visible horizontal/vertical bounds. Built both the committed candidate and its parent baseline with the pinned O3 toolchain. Used the Chis mGBA runner with `A26_STOP_ATARI_TV_COUNTER=41`, `A26_TRACE_ATARI_GLOBALS=1`, the same key sequence listed above, and a fresh copy of the same pristine shared image for each invocation. The runner now records the mGBA master-cycle count at the first cartridge-PC observation of the requested Atari counter, rather than waiting until the end of the external frame.

All three baseline and all three candidate replays stopped at 840,231,555 mGBA master cycles, external frame 2,993, Atari `clk=799673`, `clkcount=2`, `tv_counter=41`, and 40 rendered calls. Sampled minimum SP was `0x03007DD0`. All six captures had SHA-256 `c11fdf92de4b7fb70c1e23f6635b4c7527b2bcb995410443663c9126d8b20080`; all six runs had zero storage load failures. Baseline and candidate plugin SHA-256 values were respectively `ab91d38db0ad10c6c93ebdf930018815e93559d088a8a8679eef729c103d4964` and `b20a00576ef991b61db5a077189316aebace6306201828d36ad5981649c7c630`. The candidate produced no measured cycle reduction or state difference and was reverted.

These fixed-state, mGBA-derived observations reject this candidate but do not measure physical ARM7 elapsed time or audible output. All generated logs, images, and binaries remain ignored under `work/` and `source/atari2600/`.


## `draw_vector1()` IWRAM placement

A sampled guest-PC profile using `mCore::step` located most Atari-window samples in `draw_vector1()`'s per-pixel color/collision loop. Baseline linked it in Game Pak ROM at `0x0800321c`; the candidate links it at `0x03005430` in IWRAM. No function logic changed. The Chis mGBA runner measured three fresh replays per build using input events `600:0x1,1200:0x1,1800:0x80,1900:0x1`, starting at Atari TV counter 10 and stopping at 41.

Baseline plugin SHA-256 `4baab3c9a0445d5ff6be1cbe0607789def30c8572f8d449620e37c7aa522d8e5` used exactly 153,937,508 mGBA master cycles in each run; candidate SHA-256 `b8c08c211b69bdb8db6fdd6a8aa68c0705deb3088d566b23bc3344e07f7a7b72` used exactly 116,551,603 (24.29% fewer). All started at Atari `PC=0xF9F4`, `clk=182303`, `tv_counter=10` and stopped at `PC=0xF9F9`, `clk=799673`, `clkcount=2`, `tv_counter=41`. Hashes for 6502 registers, RIOT RAM/registers, TIA registers, the 62,000-byte EWRAM Atari pixel cache, and full 96 KiB VRAM matched at both boundaries. Minimum sampled IWRAM SP was `0x03007DD0` in all six runs. Candidate IWRAM static end is `0x03005D88`; the baseline ends at `0x03005BE0`.

The mGBA `getPixels()` PPM hashes differed between builds even though the emulated Atari state, EWRAM pixel cache, and VRAM hashes matched at the exact measurement boundaries. Treat that capture as scan-timing-sensitive and do not cite it as Atari pixel-parity evidence. This is a repeatable mGBA-model speed result only; actual GBA waitstate and display timing remain `UNVERIFIED`. The current candidate passes `scripts/test-baseline.sh` and all six shared-image mGBA storage lanes.
