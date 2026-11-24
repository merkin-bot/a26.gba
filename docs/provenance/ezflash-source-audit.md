# EZ-FLASH GBA-side source audit

This audit reads the exact DSSMM snapshots listed in `harnesses/cards.json`. It records software transactions emitted by those kernels. A GBA-side address or register sequence is not, by itself, proof of the card's electrical decode, response timing, waitstates, reset behavior, or physical SD implementation.

## Shared SD transaction shape

All three pinned lanes provide FatFs `disk_read`/`disk_write` through `Read_SD_sectors` and `Write_SD_sectors` in `source/Ezcard_OP.c` (BLUE's copy is under `targets/blue/source/`). Their software issues the following halfword writes:

1. Enter command mode with `0xD200` at `0x09FE0000`, `0x1500` at `0x08000000`, `0xD200` at `0x08020000`, and `0x1500` at `0x08040000`.
2. Set low and high sector address halves at `0x09600000` and `0x09620000`; set count at `0x09640000`.
3. Finish the command with `0x1500` at `0x09FC0000`.
4. Toggle the `0x09400000` control value between 1 and 3 for enable/read-state; zero disables it. Poll `0x09E00000` until its value is not `0xEEE1` (limit `0x100000` polls).
5. Read sectors by DMA from `0x09E00000`; write sectors by DMA to it. The kernel groups transfers in chunks of at most four 512-byte sectors. Writes set bit 15 in the count word. Read failure retries once after `delay(5000)`; write failure returns immediately. Successful writes delay `3000` before disabling the interface.

This gives a useful observed GBA bus transcript for an instrumented mGBA adapter. The meanings and electrical behavior of these addresses, the response value/latency, DMA interaction, SD initialization, and timeout behavior on an actual EZ cartridge remain **UNVERIFIED** by this source alone. Do not substitute the SuperCard CMD/DAT protocol.

## Cartridge windows, saves, and launch

- `ez_define.h` declares a PSRAM aperture at `0x08800000`, a NOR/flash aperture beginning at `0x09000000`, and save SRAM at `0x0E000000`. The code uses page/control writes to `0x09880000`; PSRAM and other control writes use addresses in the `0x098...` aperture. These are software assumptions made by the matching kernel, not independent hardware qualification.
- All lanes implement save-file policy in the kernel: detect or choose a GBA save mode, read/write `.sav` files through FatFs, and use SRAM/NOR-backed metadata. `Bank_Switching` emits the AMD-style byte sequence at `0x0E005555`, `0x0E002AAA`, `0x0E005555`, then writes the bank at `0x0E000000`. This source transcript does not establish which physical revisions accept it.
- Original OMEGA calls a common `SetRompage(page)` sequence, then handles boot modes in C. Plugin modes copy/decompress the cartridge image from `0x08000000` to EWRAM, reset selected GBA state, and branch to `0x02000000`; ordinary ROM reset modes use the kernel reset paths.
- OMEGA DE has a dedicated `DE_BootHandoff` assembly tail. It moves SP to IWRAM before possible EWRAM clear, validates the PogoShell size at `0x0203FBFC` for copy mode, optionally decompresses, performs BIOS register reset, and branches to EWRAM. Its page/control transaction uses the same addresses as Original OMEGA.
- BLUE adds `SYSTEM_MODE_OS=1` / `SYSTEM_MODE_GAME=0` to the page value and calls `Blue_PrepareFlashAccess()` before page/NOR operations. Its `Blue_BootHandoff` is assembly and treats native soft/hard reset, plugin copy/decompression, and plugin hard/soft modes separately. This is a source-level lane difference; actual mode-bit semantics and mapping are **UNVERIFIED** physically.
- DE, Original OMEGA, and BLUE define different reserved NOR metadata offsets. Treat these as kernel/filesystem layout facts, not evidence that a host harness should emulate NOR as ordinary RAM.

## Fixture boundary and next harness work

The reused five-case native launch fixtures execute the matching launcher/path/metadata code and reach a test handoff marker. They replace or observe storage and boot/page side effects at fixture seams. They do not run `Read_SD_sectors`/`Write_SD_sectors` against an EZ bus model, reproduce a physical card's PSRAM/NOR windows, or qualify save SRAM. Their passing state is therefore a **kernel launch-path regression**, not full GBA + EZ card + SD integration.

Next harness increment: add an EZ-specific, opt-in GBA bus trace/model separate from the SuperCard model. First assert only the kernel-emitted transcript above and report unimplemented effects explicitly. Do not claim device fidelity for synthetic response words or storage effects without matching vendor/hardware evidence. Keep the existing launch UI fixtures as-is rather than duplicating them.

## Pinned source snapshots

- OMEGA DE Rev A/B: `a948b936c11bc984ca8a84d141dc1b7ea81751a6` (`v8.4.1`)
- Original OMEGA: `29bbb055b26968468cadf7358bdffd340209a355` (`v8.4.1-omega-test5`)
- OMEGA DE B / BLUE: `9bc24e04efe4fa815c6a2d83bf319ec9ebbcd278` (`v8.4.1-blue-test5`)

The DSSMM repository is Apache-2.0 as recorded in `reference/repositories.txt`. No kernel code was copied into this project.
