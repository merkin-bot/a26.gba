# GBA, DSSMM card, and SD harness

The harness has three independent layers:

1. **GBA system:** mGBA executes the kernel/plugin GBA code and models GBA CPU, memory, timers, video, DMA, and input.
2. **Flash card and kernel:** the matching DSSMM kernel runs and interacts with a per-model card profile. Do not merge different card mappings or kernel builds into one generic profile.
3. **SD storage:** each card profile reads a block image containing that card's matching kernel/support files, the A26 plugin, and the open-source Atari ROM. FAT/filesystem operations must cross the emulated card's SD interface; a host FAT image check alone does not prove this path.

The six target profiles and exact DSSMM kernel release tags/artifacts are listed in `cards.json`. Kernels are maintained in [voluptuary/dssmm](https://github.com/voluptuary/dssmm); the earlier untagged audit snapshot `f6350776f07eba91ba2ffa34c6833d85f348f2a1` is not current release validation.

## Current SuperCard model boundary

The SuperCard adapter uses SuperFW Build769 source in `rom_boot.S`, `src/supercard_driver.c`, `src/loader.c`, and `src/supercard_io.S`. The source uses `0x09FFFFFE` for all three board builds: mode bit 0 selects SDRAM versus firmware/flash, bit 1 enables the SD interface, and bit 2 controls SDRAM writes. The GBA boot stub copies 1 KiB from `0x08000000` to IWRAM and executes it; the plugin loader stages the plugin/game at `0x08000000` and launches in SDRAM mode. The mGBA adapter now switches CPU cartridge fetches between the original kernel ROM in firmware mode and model SDRAM in SDRAM mode. This common fetch mapping is source-supported for Build769, but only the Lite launch path has been run through the current A26 SD-image runner.

The SD I/O is board-specific and is not interchangeable:

- **SuperCard Lite:** Build define `SUPERCARD_LITE_IO`; SD data reads and writes use the 32-bit stream port at `0x09200000`. The model implements this path from an optional raw block image.
- **SuperChis:** Build define `SUPERCHIS_IO`; the conditional read code shares the `0x09200000` block-port implementation, while writes use the `0x09000000`/`0x0B000000` serial registers. Its boot code also programs eight NOR map entries with a three-write sequence and uses mode bit 3 for SRAM banking. The runner models the eight-entry NOR register write sequence, but does not model NOR backing/page reads or SRAM banking.
- **SuperCard SD:** Build define `SUPERCARD_FLASH_ADDRPERM`; the SD sector read path streams from `0x09100000` or `0x0B100000`, and writes use `0x09000000` or `0x0B000000`. Its internal flash address permutation is separate from SDRAM mapping. The runner models the source-described read ports and serial write stream, but the matching Build769 SD kernel currently stops after early reads and does not reach the plugin; see `PROGRESS.md`. This profile is not integration-qualified.

The verified Lite and Chis traces each have 3,900 register values across 30 CMD18 reads (130 values per requested sector: 512 data bytes plus the 8-byte checksum); 60 file loads include one prefetch per request. Both reach the staged plugin under mGBA HLE and synthetic card/SD behavior. The SuperCard SD kernel currently decodes 13 commands and reads 520 values, then remains in the kernel; its startup path is under investigation. Host tests cover the SD read windows and serial write stream and the Chis NOR map register writes. These are source-shaped seams, not physical behavior validation. SD response timing, physical card mapping/electrical behavior, Chis NOR page contents, and SuperCard SD launch remain UNVERIFIED. Board-specific details and evidence limits are also recorded in `cards.json`.

## Reuse existing DSSMM tests

DSSMM already has native mGBA launch fixtures for all three EZ-FLASH kernel lanes. Run from `harnesses/third_party/dssmm/`:

```sh
bash ezfomegade/scripts/test-native-launch-mgba.sh
bash ezfomega/scripts/test-native-launch-mgba.sh
bash ezfblue/scripts/test-native-launch-mgba.sh
```

These execute real launch UI/path/metadata code, but mock save, FAT transfer, patch, and card boot/page side effects. They are useful kernel regressions, not complete card or SD emulation. SuperFW has native menu/renderer tests and documents that they do not initialize physical SD, NOR, or cartridge SDRAM interfaces.

## Build context

DSSMM fixtures need devkitARM. In this Codespace the verified build path is the `devkitpro/devkitarm:latest` Docker image, while mGBA/Xvfb run on the host. Absolute workspace paths matter because fixtures generate absolute include paths. A previous `build-launch/` created under `/work` can leave stale `.d` dependencies; preserve/rename those generated directories before rebuilding under `/workspaces/a26.gba`.

The local mGBA/DSSMM source checkouts are ignored by Git. Do not commit mGBA, DSSMM kernel binaries, BIOS images, card firmware, or generated SD images.

`supercard-mgba-runner <kernel.gba> [frames [sd-image [capture.ppm [key-events]]]]` accepts a kernel image and optional raw SD block image, captures a 240x160 PPM, and accepts comma-separated frame-indexed GBA key masks. Select the source-matched board using `A26_SUPERCARD_BOARD=lite|sd|chis` (default `lite`). With clean Build769 kernels and separate fresh SD images, Lite and Chis reach the staged plugin at `0x0800788A`; their 2,000-frame captures match hash `E6AFE00D3895221D`. The matching SD kernel does not yet reach the plugin. These are synthetic mGBA results, not physical card behavior, synchronized game correctness, audio, or GBA timing. Source-derived SD responses and timing remain synthetic. ARM block transfers use `ARMCore.memory.loadMultiple`/`storeMultiple`; hooks must preserve upstream transfer behavior. mGBA's `struct mCore` layout depends on private compile definitions, so harness targets inherit `$<TARGET_PROPERTY:mgba,COMPILE_DEFINITIONS>`.
