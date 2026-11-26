# StellaDS architecture audit for GBA optimization

**Audited:** 2026-11-26  
**Reference:** StellaDS Phoenix Edition commit `50dbad2f068364f6c3892f379ac4212803784607` (GPL-2.0; source in ignored `work/ref-stellads/`). This is a source review, not proof that its DS optimizations transfer to GBA hardware. StellaDS targets ARM9 DS/DSi hardware with different clock, caches, TCM, display engines, and audio ownership.

## Findings

### CPU dispatch and program-counter handling

`arm9/source/emucore/M6502Low.cpp::execute()` copies `gPC` into a local `PC`, fetches and dispatches in a switch for a batch of instructions, then writes `gPC` back when it yields. CPU registers and flags are held in DTCM. The opcode bodies are shared through `M6502Low.ins`, so execution specialization does not require duplicating all instruction semantics.

The port's `mainloop()` is also a large switch, but `PC` expands to the global `program_counter` and is referenced 387 times in `cpu.c`. A local PC is a promising semantics-preserving experiment: keep it live during the instruction loop and publish it at a carefully defined observation boundary. Before adopting it, audit every helper and shell/debug path that can read or change `program_counter`, then compare exact Atari start/stop state, framebuffer, stack, and mGBA cycle window. Merely declaring a local copy and syncing it too rarely would break observability or control flow.

StellaDS has separate `execute_4K`, `execute_F8`, `execute_F6`, `execute_F4`, and other cartridge drivers. The 4K and banked versions share opcode semantics but specialize fetches and memory accesses for their cartridge. This offers a model for conservative per-cartridge fast paths with a generic fallback. The 4K path also documents an accuracy tradeoff for games that write to ROM or depend on open-bus behavior; that shortcut must not be applied globally.

### Memory access

`System.hxx::PageAccess` supports direct read and write pointers for simple pages and device callbacks for mapped I/O or hotspots. `M6502Low.cpp` uses that table for general accesses and adds a direct zero-page RAM path. `poke_small()` likewise bypasses the general bus for known 8-bit addresses while keeping TIA writes routed through the TIA device.

The GBA plugin already does direct instruction fetch from its 4 KiB ROM buffer and places `mainloop()`, `do_screen()`, and `decRead()` in IWRAM. `decWrite()` remains in Game Pak ROM. StellaDS's safe lesson suggested fast-pathing fully decoded ordinary RAM writes while leaving TIA and cartridge writes on their existing decoder. Both inline and shared-IWRAM helper versions were measured and rejected: the inline version crowded the sampled stack, while the shared helper ran 0.111% slower in the fixed-state mGBA window. Neither fast-store change remains in the candidate.

### Code and data placement

StellaDS marks selected TIA updates, RIOT timer access, sound processing, hot cartridge handlers, and interrupt routines with `ITCM_CODE`; CPU state and pointers are placed in DTCM. On GBA, IWRAM is the analogous fast code/data region, but its capacity is a much tighter constraint. In the current candidate link, `mainloop()` is 21,552 bytes, IWRAM static use ends at `0x03007A5C`, and the measured replay's minimum SP is `0x03007DD0`. `decWrite()` is 1,644 bytes in Game Pak ROM. Moving it wholesale into IWRAM would consume stack margin and cannot be justified without a full IWRAM/stack map. The inline fast-store test also showed that code duplication can consume stack margin even when the benchmark suggests a small gain.

### TIA update, video, and frame pacing

StellaDS updates TIA output from system-cycle timestamps, renders only the display interval, and can skip selected frames for especially heavy cartridges while continuing CPU/TIA time. Its DS rendering path uses an 8-bit bitmap background, two main-memory frame buffers, asynchronous DMA of 160-byte scanline segments, and DS affine registers for crop/scale. Frame blending is an optional extra cost.

The GBA plugin instead runs an instruction-driven `do_screen()`/raster path and writes its 240x160 Mode 3 framebuffer directly. The GBA port currently uses Mode 3 (240x160, 15-bit pixels). `tv_raster1()` composes TIA objects/playfield and resolves collision and priority state on the CPU, then `draw_vector1()` converts the winning TIA color through `color[]` and stores a halfword to VRAM when its cached pixel changes. `put_image()` is a no-op; there is no full-frame copy for DMA to accelerate. Thus the GPU cannot take over 6502 dispatch, TIA register timing, collision logic, object composition, or the current color-resolution loop as written.

The tested GPU experiment was a Mode 4 indexed bitmap path: keep the CPU's exact TIA composition/collision decisions, store the final TIA color index in the framebuffer, and let the PPU's 256-entry palette perform RGB conversion during display. This removes the per-pixel software color lookup in theory, but aligned pair packing increased modeled cycles in this implementation. Bitmap BG affine parameters may support a second experiment that lets the PPU scale a source image, but it needs a new source layout preserving the full 40-bit playfield (four Atari pixels per bit), all active scanlines, and per-ROM edge crop; it is not established as useful yet. GBA Mode 4 is 240x160 indexed; Mode 5 is only 160x128 and is a poor fit for retaining the required playfield. Double buffering in Mode 4 consumes both bitmap pages, so it leaves no second page for an extra CPU source buffer; an indexed source should live in EWRAM or be rasterized directly into its final framebuffer layout.

DMA is useful only for moving already-computed pixels. Since `put_image()` currently does nothing and rendering writes directly to VRAM, adding DMA alone would add a staging copy rather than remove work. A controlled Mode 4 packed-index prototype was built and measured, then reverted. It handled GBA VRAM writes as aligned 16-bit pixel pairs because the mGBA model replicated each single-byte VRAM write across both pixels in the failed prototype; physical hardware behavior still needs qualification. Three paired-state windows were not completed, so this remains a one-run diagnostic: baseline took 153,937,508 mGBA cycles between Atari counters 10 and 41; packed Mode 4 took 164,696,222 cycles (about 7% more). The start/end Atari clocks differed by five cycles, and the capture did not match (baseline PPM SHA-256 `c11fdf92...`, prototype `815e23cc...`; pixel color counts differed by 227 pixels). The prototype is reverted and must not be treated as a speed candidate. It demonstrates that palette lookup offload is overwhelmed by aligned-pair staging in the current row layout. A future affine-scale experiment needs a separate packed source layout and exact pixel-edge mapping before it can be judged. mGBA display captures can check behavior but cannot establish physical GBA speed. The GBA PPU is a fixed-function pixel display engine, not a programmable GPU capable of emulating the Atari CPU/TIA state machine.

### Audio and portability limits

StellaDS's audio source has useful TIA polynomial tables and channel-clock logic, but its DS audio path also uses ARM7/DS sound facilities and DS-specific VRAM buffers. It cannot establish GBA DMA timing or fix the measured emulation/audio clock mismatch by itself. The 4/5-bit polynomial counters are being aligned with the pinned reference; the current 9-bit generator still uses an LCG and its hardware fidelity is `UNVERIFIED`.

## Measurement: direct RIOT RAM store fast path (rejected)

StellaDS's `poke_small()` and direct page mappings motivated the GBA `fast_store()` helper in `source/atari2600/macro.h`. It bypasses `decWrite()` only where the existing decoder writes `theRam[a & 0x7f]`; all cartridge, TIA, and other writes still call `decWrite()`.

Compared baseline plugin SHA-256 `4baab3c9a0445d5ff6be1cbe0607789def30c8572f8d449620e37c7aa522d8e5` to candidate `9246d04765b52ca7a65adc246d574e59d520f57fae5169d0b57964c0580e135a`. Both were built with the same audio-table changes, staged into fresh 4 GiB exFAT images with identical paths, and run on the Chis mGBA model using a 3,200-frame replay and the same key events. Measurement begins at Atari TV counter 10 and ends at 41, with offsets verified against each linked ELF.

Three baseline runs each used 153,937,508 mGBA master cycles between those Atari boundaries. Three candidate runs each used 153,655,973 cycles, a reduction of 281,535 cycles (0.183%). Start PC, `clk`, and TV counter matched exactly (`PC=0xF9F9`, `clk=182308`, counter 10); the end state matched (`clk=799673`, counter 41), all six framebuffer hashes were `c11fdf92de4b7fb70c1e23f6635b4c7527b2bcb995410443663c9126d8b20080`, minimum sampled SP remained `0x03007DD0`, and storage load failures were zero. This small repeatable result is mGBA-model evidence, not measured physical GBA speed.

## Source pointers

- [M6502Low.cpp at the audited revision](https://github.com/wavemotion-dave/StellaDS/blob/50dbad2f068364f6c3892f379ac4212803784607/arm9/source/emucore/M6502Low.cpp): local PC, direct access helpers, and per-cartridge CPU drivers.
- [System.hxx at the audited revision](https://github.com/wavemotion-dave/StellaDS/blob/50dbad2f068364f6c3892f379ac4212803784607/arm9/source/emucore/System.hxx): direct page mappings.
- [TIA.cpp at the audited revision](https://github.com/wavemotion-dave/StellaDS/blob/50dbad2f068364f6c3892f379ac4212803784607/arm9/source/emucore/TIA.cpp): cycle-driven frame update and cartridge-specific dispatch.
- [StellaDS.cpp at the audited revision](https://github.com/wavemotion-dave/StellaDS/blob/50dbad2f068364f6c3892f379ac4212803784607/arm9/source/StellaDS.cpp): DS frame pacing, DMA/display setup, and affine scaling.


### GBA OBJ hardware as a possible TIA object compositor

The AGB Programming Manual permits OAM/OBJ VRAM access during H-Blank only when OBJ processing for that interval is disabled, and notes that OBJ processing performance is reduced in that configuration. H-Blank DMA to OAM/OBJ VRAM also requires freeing H-Blank from OBJ display processing. A TIA player's GRP/position/size registers may change at raster-timed CPU writes; the current `tv_raster()` callback advances on emulated Atari time and is not synchronized to the GBA LCD's current scanline. Therefore, a CPU-generated H-Blank OAM stream cannot simply mirror TIA raster events. Player/missile/ball sprites could theoretically be represented as GBA OBJ layers for a subset of games, but CPU collision masks and TIA object priority would still need to be computed, and arbitrary mid-line writes would require a new synchronization/line-buffer design. This path is not a safe low-risk offload and has no prototype or timing result.

## Follow-up: GBA PPU/DMA offload review

The official [Nintendo AGB Programming Manual](https://mutantstargoat.com/~nuclear/tmp/gba_prog_manual.pdf) describes bitmap BG modes 3-5, Mode 4's 8-bit palette-index framebuffer, BG2 affine rotation/scaling, and the H-Blank restrictions on OAM/OBJ VRAM access. This supports palette lookup and display transform offload, but not general-purpose pixel processing. The PPU review is based on the manual's rendering-function sections and source inspection of this port. One Mode 4 packed-pixel prototype was attempted and rejected for slower cycles and image mismatch; physical hardware timing remains unverified.

## Follow-up: shared helper replay

Tested a single IWRAM `fast_store()` helper to avoid inline-code growth. With matching audio-table builds and fresh 4 GiB exFAT images, three baseline runs each took 153,937,508 mGBA cycles from Atari counter 10 to 41; three helper runs each took 154,107,903 cycles (0.111% slower). All six PPM hashes matched (`c11fdf92...`), SD write failures were zero, and the sampled minimum SP was `0x03007DD0` baseline / `0x03007DC8` helper. The helper is reverted. Although its linked IWRAM end (`0x03005BE0`) was below that sampled stack position, it did not improve the measured work. This is mGBA model evidence only.

The StellaDS-style local-PC experiment was also attempted by substituting a local register in `cpu.c`; the link exceeded IWRAM, ending static data at `0x03009524`, about 5 KiB beyond the region, so it was reverted.
