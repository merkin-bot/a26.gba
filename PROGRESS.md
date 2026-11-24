# PROGRESS.md

Handoff document for any AI agent or human continuing this project. Read `AGENTS.md` first, then this file in full.

**Last updated:** 2026-11-24 UTC by Codex
**Current branch:** main
**Last pushed commit:** See `git log -1 --oneline`; the commit before current edits was `ffe12d2`.

> Rules: update this file before starting each task and right after finishing each step. Commit and push it often. Delete stale info rather than piling up contradictions. Mark anything not confirmed as `UNVERIFIED`.

---

## 1. Project summary

- **Goal:** Atari 2600 emulator for the Game Boy Advance, running as a PogoShell plugin on EZ-FLASH Omega family and SuperCard family devices. Continues https://github.com/voluptuary/dssmm.
- **Status:** Runs on unpublished SuperCard build 822, but slowly, with no sound, and menu parity with other PogoShell plugins (e.g. Goomba) is incomplete (different menu font). Existing source archive has a prepared GPLv3 source build, an experimental plugin binary, an open-source generated diagnostic, and runtime checks from stock mGBA 0.10.1.
- **Current focus:** Harness-first validation against latest published DSSMM kernels; plugin optimization remains gated on harness qualification.

---

## 2. Quick start for a new agent

1. Read `AGENTS.md`, `HANDOFF.md`, then this file.
2. `git status` and `git log -5 --oneline`; reconcile with section 6.
3. If the environment is fresh, follow section 3.
4. Run the build and test commands in section 3.4 to confirm the baseline.
5. Resume the item under "In progress" in section 6 using its resume command.

---

## 3. Dev environment setup (GitHub Codespaces)

### 3.1 Base environment
- Devcontainer: `.devcontainer/devcontainer.json`
- Setup script: `.devcontainer/setup.sh`
- Base image: `mcr.microsoft.com/devcontainers/base:ubuntu-24.04`

### 3.2 Toolchain
- devkitPro / devkitARM version: Docker `devkitpro/devkitarm:latest`, compiler `arm-none-eabi-gcc (devkitARM) 16.1.0`; exact image digest not pinned
- Install method: `.devcontainer/setup.sh` calls the devkitPro pacman installer, then `dkp-pacman -Sy gba-dev libgba`
- Environment variables: `DEVKITPRO=/opt/devkitpro`, `DEVKITARM=/opt/devkitpro/devkitARM`, PATH additions: `$DEVKITARM/bin:$DEVKITPRO/tools/bin`
- apt packages: build-essential, cmake, ninja-build, SDL2/Qt/OpenGL, PNG/ZIP/ZLIB, FFmpeg, ELF/edit, plus devkitPro `gba-dev`

### 3.3 Repo contents and third-party assets
- The original archive was ingested and removed; assets are organized under `dist/`, `test-roms/`, `source/`, `docs/provenance/`, and `reference/`
- Prepared canonical source: `source/atari2600/`
- PogoShell and other required third-party binaries: where to get them, where to place them, license notes. **Do not commit** anything whose redistribution is not permitted.
- Open-source test game: `test-roms/flappy.a26`, source/license: archive-provenance `JetSetIlly/Atari2600-Flappy-Bird` BSD-2-Clause

### 3.4 Build and test commands
- Build plugin: `<TODO>`
- Build plugin: `bash scripts/build-plugin.sh` (official `devkitpro/devkitarm:latest` Docker image)
- Build standalone stock-mGBA fixture: `bash scripts/build-fixture.sh`
- Build/verify FAT32 flashcard SD: `bash scripts/verify-harness.sh`
- Build patched mGBA (with SuperCard/EZ-Flash harnesses): `<TODO>`
- Run stock mGBA headlessly: `bash scripts/run-mgba.sh <gba-rom> <seconds>`
- Run baseline: `bash scripts/test-baseline.sh`

### 3.5 Codespaces limitations hit
- No USB passthrough to real hardware, storage limits, idle timeout, etc.: <TODO, add as discovered, with workarounds>
- apt devkitPro installer URL was blocked with HTTP 403/Cloudflare; official Docker image `devkitpro/devkitarm:latest` is the verified fallback.

---

## 4. Repository map

| Path | Purpose |
|------|---------|
| `dist/a26.gba` | Experimental plugin binary |
| `test-roms/flappy.a26` | Open-source Atari 2600 test ROM |
| `docs/provenance/` | Test and runtime provenance |
| `reference/repositories.txt` | Source of repository research links |
| `AGENTS.md` | Standing instructions for agents |
| `PROGRESS.md` | This file |
| `HANDOFF.md` | Short crash-recovery snapshot |
| `source/atari2600/` | Prepared GPLv3 source build for the experimental plugin |
| `.devcontainer/` | Reproducible Codespaces environment setup |

### Referenced repositories (from `reference/repositories.txt`)

| Repo | What it is | License | Used for |
|------|-----------|---------|----------|
| [MikeDX/2600_gba](https://github.com/MikeDX/2600_gba) | Starting upstream; legacy GBA emulator source | GPLv2 according to GitHub metadata but source archive notes GNU GPL without numbered version | Starting point, CPU/core/TIA/legacy build |
| [Sterophonick/PocketStella](https://github.com/Sterophonick/PocketStella) | GBA Atari 2600 emulator | GPLv2 according to GitHub metadata | GBA compatibility/performance/PogoShell/GBA port techniques |
| [wavemotion-dave/StellaDS](https://github.com/wavemotion-dave/StellaDS) | Nintendo DS Atari 2600 emulator, Phoenix edition | GPL-2.0 metadata/owner authorization | CPU/TIA/audio/optimization research |
| [stella-emu/stella](https://github.com/stella-emu/stella) | Reference Atari 2600 emulator | GPLv2 according to GitHub metadata | Accuracy and reference behavior |
| [JetSetIlly/Atari2600-Flappy-Bird](https://github.com/JetSetIlly/Atari2600-Flappy-Bird) | Open-source Atari 2600 Flappy Bird source/game | BSD-2-Clause according to GitHub metadata | Compatibility test ROM candidate |

---

## 5. Hardware/software knowledge base

Each fact needs a source or evidence. Separate verified from unverified.

### VERIFIED
- The original archive contents were ingested and the archive removed; canonical assets are in `dist/`, `test-roms/`, `source/`, `docs/provenance/`, and `reference/`.
- Prepared source reproduces the archived plugin SHA-256 `14d28e63...f4500` with devkitARM 16.1.0 (source: Docker build and `sha256sum`).
- Prepared plugin source is GPLv3-or-later adapters/menu with upstream GPL legacy code; `COPYING` and `NOTICE.txt` are retained (source: archive).
- Stock Ubuntu mGBA 0.10.2 is available at `/usr/games/mgba` and Xvfb is installed (source: apt).
- GitHub metadata identifies MikeDX/2600_gba and Stella as GPL-2.0, StellaDS is GPL-2.0, and Flappy Bird BSD-2-Clause. For any referenced repository with no license metadata, assume GPL-2.0 and verify before copying (source: GitHub API and owner authorization).
- Existing prepared plugin supports 2K/4K/8K F8/16K F6 only; no TIA audio implementation is present (source: `atari2600-test.txt`, source search).
- PogoShell argument block is fixed at EWRAM `0x0203fbf8` and ROM handoff validates a known ABI marker (source: `startup.S`, `files.c`).
- A hardware SuperCard 822 trial and compatibility claims beyond the diagnostic are unqualified (source: `atari2600-test.txt`).
- A standalone `A26_FIXTURE` GBA image embeds `test-roms/flappy.a26` and survives 3 seconds under stock Ubuntu mGBA 0.10.2/Xvfb (source: `scripts/test-baseline.sh`).
- Public PocketNES/GoombaColor source confirms PogoShell launch at `0x0203FBFC` and tail at `0x0203FBF8`, plus ROM filename parsing from `0x0203FC08` (source: public GPL/Github references; license verify per-file before reuse).
- EZ-FLASH Omega kernel public metadata is Apache-2.0 (source: GitHub API); it is a kernel reference only, not evidence of physical cartridge behavior.
- A 64 MB FAT32 SD image is verified with `a26.gba` under `SYSTEM/PLUG/` and `flappy.a26` under `ROMS/`; plugin and ROM hashes match source assets (source: `scripts/verify-harness.sh`).
- A stock-mGBA PogoShell cold-boot fixture seeds `0x0203FBF8/FC`, appends Flappy, and survives 5 seconds; this proves portable ABI execution, not card firmware/SD/NOR behavior (source: `scripts/run-card-fixture.sh`).
- DSSMM SuperFW source confirms portable PogoShell ABI argument layout and SuperCard cartridge-SDRAM/NOR/SD boundaries, but explicitly states stock mGBA cannot emulate full card I/O (source: `voluptuary/dssmm/superfw/docs/MGBA-TESTING.md`, Apache-2.0).
- EZ-FLASH Omega Definitive kernel source is public and Apache-2.0; PogoShell detection is code-confirmed at `0x0203FBFC`/`0x0203FBF8` and filename parsing at `0x0203FC08` (source: `ez-flash/omega-de-kernel`).
- SuperCard build 822 kernel/register/SD implementation remains UNVERIFIED; no redistributable kernel or public register evidence was found.

### UNVERIFIED
- PogoShell ABI details, return stub layout, and save memory behavior beyond the source's fixed argument block (how to verify: obtain PogoShell/plugin source or a real firmware experiment).
- SuperCard/EZ-Flash Omega harness behavior and kernel remapping; no existing harness patches were found in this repository (how to verify: build patched mGBA and compare with vendor/kernel evidence).
- Real-hardware speed and `60 fps` feasibility (how to verify: baseline timer/frame instrumentation and physical GBA measurement).
- `flappy.a26` provenance and license: archive does not include a notice for this file (how to verify: identify upstream source/license; no commercial ROM).

Topics to cover: PogoShell plugin ABI (load, launch, ROM handoff, save/state memory, return to shell); EZ-Flash Omega and SuperCard/SuperCard behavior (SD/FAT access, save types, waitstates); memory layout used by the plugin; timing budgets (cycles per scanline/frame); TIA audio plan (Direct Sound/PSG).

---

## 6. Task board

### In progress
- **Task:** Complete the evidence-based SuperCard host model and proceed toward mGBA card/SD integration.
- **Exact step:** Added source-shaped synthetic CMD55/ACMD41, CID/CSD, RCA, card-select, bus-width and block-length responses. Host model and mGBA smoke pass. Clean Lite Build769 trace reaches CMD18 then CMD12, with 13 decoded commands, 10 responses, 666 modeled response-line reads, no DAT/block reads, and stable failure by frame 1,800/2,400. Next add Lite 32-bit data-register input backed by an optional SD image and verify the first block-read path; do not call this a completed card harness. SD timing/physical compatibility remain UNVERIFIED.
- **Files touched:** `AGENTS.md`, `HANDOFF.md`, `PROGRESS.md`, `harnesses/CMakeLists.txt`, `harnesses/README.md`, `harnesses/include/supercard_mgba_bus.h`, `harnesses/src/supercard_mgba_bus.c`, `harnesses/src/mgba_core_smoke.c`, `harnesses/src/mgba_supercard_runner.c` plus model correction in commit `5e62f89`.
- **Uncommitted changes:** SD idle-line model/hook, register-read smoke assertions, runner diagnostics, and refreshed release confirmation. DSSMM/mGBA clones and generated outputs are ignored.
- **Branch:** main
- **Resume command:** Build/check with `cmake -S harnesses -B build/harness-cmake -G Ninja && cmake --build build/harness-cmake --target supercard-model-test mgba-core-smoke supercard-mgba-runner && build/harness-cmake/supercard-model-test && build/harness-cmake/mgba-core-smoke`. Run the headless diagnostic as `build/harness-cmake/supercard-mgba-runner <local-gba-image> <frames>`. For EZ fixtures checkout the tag in `harnesses/cards.json`; for SuperFW always `make clean` before switching boards.

### Next up (ordered)
1. Extend `supercard_mgba_bus` for source-confirmed SD mirror reads/writes and implement/test SD command response initialization using the SuperFW bit-banged CMD/DAT routines; then add block reads/writes backed by per-board image files.
2. Add the matching EZ-FLASH cartridge/SD interface models using DSSMM source and existing kernel fixtures; do not assume SuperCard protocol applies.
3. Exercise PogoShell kernel launch, plugin loading, ROM handoff, save and return paths through each matching card profile and an SD image.
4. Keep mGBA harness behavior separate from plugin workarounds; document hardware gaps.
5. After harness verification, resume plugin timing/optimization with responsive input and stable 30 fps plus audio sync as targets; compare Stella/plugin output at synchronized states.

### Backlog
- Headless screenshot/frame-hash capture
- PogoShell return and ROM ABI harness in upstream mGBA

### Done
| Date | Task | Commit | Notes |
|------|------|--------|-------|
| 2026-11-24 | Initial audit and reproducible build baseline | cf5e58f | Extracted archive, created handoff, Docker build reproduces plugin SHA-256 |

---

## 7. Performance log

| Date | Commit | Test ROM | Metric | Method | Result |
|------|--------|----------|--------|--------|--------|
| 2026-11-24 | pending | Flappy/standalone fixture | screenshot SHA-256 | stock mGBA0.10.2/Xvfb/F12 | `829ba115435c124a40bcad650c6ec61ac2c8c747b813b8aa32fb525c5ff653c0`; F12 PNG 240x160 now captured directly; startup/state alignment still unverified |
| 2026-11-24 | pending | Flappy/Stella desktop | scaled screenshot SHA-256 | Stella + Xvfb, rendered bounds crop, nearest resize | `f729700554bcf6027da47a7b062391522024e23a75f2b89018c0c15f5817bba2`; mapped into 160x90 game viewport at (35,25) on 240x160 canvas; full source screenshot is `work/reference/stella-full.png` |
| 2026-11-24 | placement-candidate | none/generated fixture | `.text`/`.iwram` layout | ELF size | Before: 30,944-byte `.text`; candidate: 12,464-byte `.text`, 18,752-byte `.iwram` |
| 2026-11-24 | arm-hot-candidate | generated fixture | hot symbol size/frame hash | ELF nm + mGBA capture | `mainloop` 17,968→27,996 bytes; `do_screen` 284→496; `tv_raster` 260→380; frame hash unchanged `47fb...c15e`; reverted without timing proof |
| 2026-11-24 | candidate-build | generated PogoShell fixture | SHA-256 | devkitARM build | `b82e459989bca244e46496fde488d71d8463f263594d26fd53c4a02f763bb277` |
| 2026-11-24 | real-hardware-layout-candidate | generated PogoShell fixture | SHA-256/frame | devkitARM + mGBA | `5e0e1f69b9770b7fc135870e9bf59d2509adbc45897a76d857da4dfc5ec5c759`; frame unchanged |
| 2026-11-24 | collision-table-candidate | generated PogoShell fixture | SHA-256/frame | devkitARM + mGBA | `ecca333d6ecb0c0357315bcd3c6628a6e423067aea760fc281e5f212b180eaf7`; frame unchanged |
| 2026-11-24 | queue-early-exit-candidate | generated PogoShell fixture | SHA-256/frame | devkitARM + mGBA | `ea0bacfd8eb9e4b2e987378111fb07a805a3f4ed2c712156ec5ff2bd26bfa1df`; frame unchanged |
| 2026-11-24 | 30fps-default-candidate | generated PogoShell fixture | SHA-256/frame/ELF | devkitARM + mGBA | SHA-256 `b3b6e8067a1d8ec313353ab62b0325fa3cc964fef192dea0d847ddd24cd46b31`; frame unchanged; 209,494 bytes ELF |
| 2026-11-24 | opcode-fetch-candidate | generated PogoShell fixture | SHA-256/frame | devkitARM + mGBA | `cc868252fb113cd4160b8c6677d39cfe076056a4422f262b0975c99f5fb21de7`; frame unchanged |
| 2026-11-24 | audio/menu-candidate | generated fixture | ELF total/text/sbss + frame hash | devkitARM size + mGBA capture | Total 202,874→207,046 (+4,172); `.text` 30,944→32,344 (+1,400); `.sbss` 155,184→157,232 (+2,048); frame hash unchanged |
| 2026-11-24 | goomba-font-candidate | generated fixture | menu/game frame hash | mGBA F12 + L+R | Menu frame `863be190...0682`, game frame `47fbde60...c15e`; menu differs as expected |

---

## 8. Test status

| Test/harness | How to run | Status | Notes |
|--------------|-----------|--------|-------|
| Reproducible plugin build | `bash scripts/build-plugin.sh` | PASS | Candidate SHA-256 recorded; `dist/a26.gba` is reference-only |
| Stock mGBA standalone fixture | `bash scripts/test-baseline.sh` | PASS | Fixture survived 3 seconds under stock mGBA 0.10.2/Xvfb |
| Stock mGBA plugin launch | `bash scripts/run-mgba.sh <plugin> <seconds>` | NOT VALID | Plugin rejected as incompatible because PogoShell ABI is absent |
| FAT32 virtual SD | `bash scripts/verify-harness.sh` | PASS | 64 MB FAT32, plugin and Flappy hashes verified |
| PogoShell portable ABI | `bash scripts/test-baseline.sh` | PASS | Layout and argument-block assertions pass; no card I/O |
| PogoShell cold-boot fixture | `bash scripts/run-card-fixture.sh 5` | PASS | Actual plugin fixture survived 5 seconds in stock mGBA; card I/O excluded |
| TIA audio host mixer | `build/tia-audio-test` | PASS | Volume-only, AUDF, and DIV3 smoke tests; no hardware/audio waveform qualification |
| TIA audio/menu implementation | `bash scripts/run-card-fixture.sh 3 && bash scripts/capture-fixture.sh` | PASS | Cold boot and frame hash unchanged; ELF cost measured |
| PogoShell real-layout helper | `build/pogo-layout-test` | PASS | Cartridge offset, table offset, and ABI block bounds |
| TIA collision lookup | `build/collision-table-test` | PASS | 64-entry object-pair lookup plus cold-boot/frame parity |
| 30 fps default cadence | `app_data.rr=2` | PASS | Every-second-frame render target; CPU/TIA/RIOT core continues at its emulated clock |
| SuperCard mode/SDRAM/SD host model | `cmake -S harnesses -B build/harness-cmake -G Ninja && cmake --build build/harness-cmake --target supercard-model-test && build/harness-cmake/supercard-model-test` | PASS | Source-evidenced two magic + two equal mode writes, per-model state, access gates, SD idle-line mirrors and block-file operations. SuperFW source only; not physical-card qualified. |
| EZ-FLASH Omega card harness | `<TODO>` | PARTIAL | Omega source exists; kernel launch and card register fixture remain |
| SuperCard 822 card harness | `<TODO>` | PARTIAL | DSSMM SuperFW source provides portable ABI and SuperCard boundaries; card protocol evidence still limited |
| mGBA core + SuperCard register hook smoke | `build/harness-cmake/mgba-core-smoke` | PASS | Static mGBA ABI definitions must be inherited; exercises mode writes and CMD/DAT idle reads, not card response/ROM/block mapping. |
| mGBA SuperCard headless diagnostic | `build/harness-cmake/supercard-mgba-runner <rom> <frames>` | PARTIAL | Clean Lite Build769 reaches CMD18/CMD12 after synthetic initialization responses: 13 decoded commands, 10 responses, 666 response-line reads, zero data-register/block reads; mode `0x0007`. No SD block stream or real storage image is connected. |

### Known harness-vs-hardware differences
- Stock mGBA cannot yet validate PogoShell ABI, firmware cartridge remapping, or physical Exit; `atari2600-test.json` uses a generated ROM and a return marker stub in stock SDL mGBA.
- A virtual FAT32 SD proves file staging only; it does not emulate FPGA/NOR/controller/register behavior or real FAT latency.

---

## 9. Known issues and decisions

### Issues
- <bug, repro steps, suspected cause>

### Dead ends (do not repeat)
- Direct devkitPro apt installer endpoint at `apt.devkitpro.org` returned HTTP 403 in this environment; use official Docker image or a host-specific installer.
- IWRAM placement candidate moved `mainloop`, `do_screen`, and `tv_raster` into `.iwram`, changing `.text` 30,944→12,464 bytes and `.iwram` 0→18,752 bytes, but the plugin hash changed and speed was not measured. Reverted; no claim of improvement.
- ARM hot-function candidate grew `mainloop` 17,968→27,996 bytes and preserved frame hash and cold-boot survival, but speed was not measured and the plugin hash changed. Reverted.
- Profile scaffold attempted Timer0/1 plus frame counter but produced no cycle metric and was incomplete. Reverted; normal hash restored.
- First static-core smoke compiled but crashed because the consumer omitted mGBA's private ABI compile definitions. gdb showed a null function-pointer call after initialization; propagating `TARGET_PROPERTY:mgba,COMPILE_DEFINITIONS` fixed it. `mgba-core-smoke` now passes initialize/reset/deinitialize and mode-register hook checks. Do not repeat the earlier diagnosis that mGBA core initialization itself fails.

### Design decisions
- Official `devkitpro/devkitarm:latest` Docker image is the default build path because it produced the exact archived plugin hash and avoids broken host bootstrap.
- Keep generated build outputs outside Git; canonical assets live under `dist/`, `test-roms/`, `source/`, `docs/provenance/`, and `reference/`.
- Use a standalone fixture only for core smoke tests; never treat stock-mGBA PogoShell simulation as proof of card firmware behavior.

### Licensing notes
- Archive prepared source retains GPLv3; legacy upstream code is GNU GPL as stated in source/CREDITS without a numbered version in some files.
- MikeDX/PocketStella/Stella GitHub metadata says GPL-2.0; Flappy Bird metadata says BSD-2-Clause. For referenced repositories with no license metadata, assume GPL-2.0 for planning and implementation use; verify the repository license file before copying and retain attribution/notices.
- PocketNES and GoombaColor PogoShell ABI usage was studied from public sources; no code was copied into this repository.
- EZ-FLASH Omega kernel is Apache-2.0, mGBA/MPL-2.0, and the relevant EZ-FLASH Air reference harness is Apache-2.0; no third-party implementation code was copied yet.
- StellaDS TIA sound timing concepts are GPL-2.0 and attributed in `tia_audio.c`; the GBA Direct Sound adaptation is a derived implementation and remains an accuracy/performance candidate.
- **Permission:** Owner authorization received to use any code at https://github.com/wavemotion-dave/StellaDS. Integration will preserve GPL-2.0 attribution and adapt only concepts suitable for 16.78 MHz GBA, not DS hardware/buffering assumptions.
- Permission received to use the Goomba font; 86 compressed Goomba font tiles are retained for menu parity.

---

## 10. Open questions for the owner

1. <question> (blocked tasks: <list>; working on <other task> meanwhile)

---

## 11. Session log

Short dated entries. Trim old entries periodically.

- 2026-11-24: Read AGENTS/PROGRESS; ingested and removed `a26.zip`; organized source/test/assets and reference repositories; created HANDOFF and devcontainer scripts; verified Docker devkitARM 16.1.0 reproduces archived SHA-256 `14d28e63...f4500`; installed stock mGBA 0.10.2/Xvfb. Next: implement native baseline checks and PogoShell/mGBA harness.
- 2026-11-24: Added GPL-2.0-attributed StellaDS-adapted TIA audio state/divider/poly mixer and GBA Direct Sound scaffold. Host mixer tests and 3-second PogoShell/frame checks pass; hardware sound accuracy and performance remain unqualified.
- 2026-11-24: Resumed at commit `85f8141`; baseline and plugin build completed. Corrected Stella and mGBA F12 image capture; both output nonblank 240x160 frames, but game states are not synchronized. No speed improvement claimed.
- 2026-11-24: Owner clarified harness-first sequencing: finish and verify hardware-faithful mGBA harness before plugin optimizations; never change harness to compensate for plugin issues. Harness must represent (1) GBA hardware, (2) each of three EZ-FLASH and three SuperCard variants, and (3) each card’s SD storage with plugin and Atari ROM. Audit found CMake references absent `harnesses/src/mgba_supercard_runner.c`; the standalone SuperCard host model is not integrated with mGBA. mGBA upstream pinned for local investigation at `431935616c0e216d650b25b2ea0c0acccd359038` (MPL-2.0); ignored clone lives at `harnesses/third_party/mgba`. SuperFW source pinned locally at `53531e70af100a53dc0a273f206e68666f04fc8d`; license file absent from checkout, so no code copied. Source inspection confirms SuperFW toggles mode register `0x09FFFFFE` using two writes of `0xA55A` then two writes of mode; source docs describe SDRAM/flash mapping and SD occupying upper 16 MiB. This evidence is for SuperFW-supported cards, not SuperCard 822 or EZ-FLASH. Exact six SKU names and differences remain UNVERIFIED; do not assume a shared profile. Exact EZ lanes: OMEGA DE Rev A/B uses stable DSSMM 8.4.0 (`ezkernelnew.bin`); Original OMEGA uses experimental 8.4.0 OMEGA Test 4 (`ezkernel.bin`); DE B/BLUE uses experimental 8.4.1 BLUE Test 4 (`ezbluekernel.bin`). SuperFW Build 667 has Chis, SD, Lite builds; earlier Chis Prime v1.0 smoke tests exist, but Build667 needs physical qualification and SD/Lite are untested. Exact profiles are in `harnesses/cards.json`. DE/OMEGA ignored test records are under `harnesses/third_party/dssmm/{ezfomegade,ezfomega}/build/native-launch/`. Finish BLUE cases, then design only missing GBA/card/SD integration.

- 2026-11-24: Owner clarified all target cards use the matching DSSMM kernels and requested reuse of that repository’s GBA/SuperCard/EZ mGBA tests. Pinned snapshot `f6350776`; found three EZ native launch suites plus SuperFW renderer/menu fixtures. Their docs explicitly say physical SD transfer, saves, patching, boot/page effects are mocked; full card+SD is not emulated. Added six-profile matrix and three-layer harness description; no DSSMM code copied. EZ DE/Original/BLUE native launch scripts are being run with Docker devkitARM and host mGBA; first attempt failed due stale `/work` dependency paths.

- 2026-11-24: Reused DSSMM native launch suites. `ezfomegade` and `ezfomega` each passed five mGBA launch scenarios (FAT alias, exFAT, long name, quick clean, quick addon), reaching mocked cartridge handoff. Actual runtime was mGBA 0.10.2, Xvfb, no BIOS; generated DSSMM JSON labels 0.10.1 incorrectly. BLUE first build hit stale `/work` dependencies; the nested `ezfblue/targets/blue/build-launch` directory was preserved/renamed and rerun is next. These tests do not validate physical SD/card protocols.
- 2026-11-24: Owner reminded to include all SuperCard variants. Matrix already lists DSSMM SuperFW Build667 profiles for Chis/Chis Prime, SD, and Lite. SuperFW has existing native menu/settings/renderer tests, but its own `MGBA-TESTING.md` says these do not initialize physical SD/NOR/SDRAM. Next after BLUE: inspect per-board build/test switches and reuse those tests; record which one exercises kernel code versus presentation fixture, then implement missing emulated card bus only against source evidence.
- 2026-11-24: DSSMM BLUE native mGBA launch suite passed all five cases after preserving its old `targets/blue/build-launch` output with stale `/work` dependency paths. FAT, exFAT, long name, quick-clean and quick-addon reached mocked page `0x400` handoff. All three EZ suites now pass; hardware/card SD remains unverified. Next cover SuperFW Build667 board builds and existing mGBA native menu/settings fixtures for Chis/SD/Lite.
- 2026-11-24: Updated card matrix from release page tags and full commit IDs; prior results were explicitly demoted to older snapshot evidence. Checked out published DE `v8.4.1` and reran all five fixture scenarios (FAT-SFN, exFAT, long exFAT name, quick-clean, quick-addon). All report “all launch stages reached handoff,” source commit `a948b936...`, traces `d7ffaa...` (quick-addon `d7bfaa...`). Runtime was mGBA 0.10.2; generated metadata still says 0.10.1. Mock seams and lack of hardware qualification remain unchanged. Next: Original OMEGA Test 5.
- 2026-11-24: Published Original OMEGA `v8.4.1-omega-test5` also passed all five reused launch scenarios on mGBA 0.10.2, reaching mocked `0x200` handoff. The run was at source `29bbb055...`; metadata continues to claim mGBA 0.10.1. This validates only DSSMM launcher/path/metadata behavior at the fixture boundary, not physical card or SD behavior. Next: BLUE Test 5.
- 2026-11-24: Published BLUE `v8.4.1-blue-test5` passed all five reused launch scenarios on mGBA 0.10.2, reaching mocked `0x400` handoff. The run was at source `9bc24e04...`; metadata continues to claim mGBA 0.10.1. All current EZ release fixtures now pass, but they mock storage/boot seams and do not qualify physical hardware. Next: SuperFW Build769 per-board builds and tests.
- 2026-11-24: Built published SuperFW `superfw-build769` at `37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4` for all three targets through devkitARM Docker. Chis ROM SHA-256 `c987108d91682394451d5217a6cbb74fa3f3a7a63664b47cfd798e4af7c1f5fd` (1.6 MiB); SD `c9ce3f78a8ef3cdf05ab2354885ab59cac2b0803282fefff55a7b9569f28fccf` (493 KiB); Lite `45adebcbcfa219c9dfec569124d7a58d72dbe282e3d8b2d0f172f13e1fa6cc6d` (493 KiB). Ignored copies saved under DSSMM `superfw/build/release-builds/superfw-build769/`. All builds pass; no physical card qualification. Next run existing native menu/settings mGBA fixtures.
- 2026-11-24: Reconciliation found the preceding sequential SuperFW matrix command did not clean between `BOARD` values. Since Make reuses `firmware.ewram.*` across profiles without encoding `BOARD` in the target path, SD/Lite might contain Chis/previous-board intermediate code. Discard the SD/Lite hashes above and clean each profile before rebuilding; no SD/Lite build success is claimed until then. Chis-native settings two-boot and 12 renderer scenes plus four Chis NOR carousel scenes passed against linked fixture code. These use virtual SRAM/ROM fixtures and do not model the SuperCard SD bus.
- 2026-11-24: Rebuilt all three SuperFW Build769 profiles after `make clean` before each target, confirmed profile macros in compiler commands, and saved separate ignored artifacts. Clean hashes: Chis `7fa0e67e66345b05d6ffe9896c2dfc10f0095877e4b2cd8664ad13e15d999d24`, SD `89786c48771c8df7b73c2009af4db74699a7147bafad1d2c21e39acd74d6fecf`, Lite `476e9d5fef4b07cc2b3a59476bae7692ea7405ab41fb92166489778b5a48c283`. Each compile line had the expected board define and Build769 version words. Clean-built each again and ran native settings mGBA fixture against board-specific linked EWRAM firmware; Chis, SD, Lite each passed two boots using virtual SRAM-backed SD. Chis also passed 12 native menu visual scenes and four NOR carousel scenes. Physical bus/SD/NOR timings remain unverified.
- 2026-11-24: Source audit found the host SuperCard model was wrong: DSSMM `supercard_driver.c` writes `0xA55A` twice to `0x09FFFFFE`, then the same mode word twice, while our model accepted one magic plus one mode. Corrected the per-model register state, gated SDRAM writes on SDRAM+write access and block-file ops on SD mapping/write bits, and added size/seek overflow checks. Strict C11 host compile/test and CMake configure/build/run pass. CMake now skips the absent mGBA runner instead of failing before the host model can be tested. Evidence is DSSMM SuperFW Build769 only; Chis NOR-map and physical bit-banged SD commands remain unmodeled.
- 2026-11-24: Built pinned mGBA 4319356 as a static library. Added `mgba-core-smoke` and discovered its first runtime crash was an ABI mismatch: mGBA's `mCore` fields depend on private target definitions. Propagated the definitions, then core init/reset/deinit passed. Added `supercard_mgba_bus`, which wraps `ARMCore.memory.store16` for the mode-register address; smoke writes the exact four-write mode sequence through `core->busWrite16` and verifies the card state. Host model and smoke pass in CMake. This is a mode-register hook only; card ROM mapping, SD CMD/DAT, timing, kernel boot and plugin/ROM launch are still unimplemented.
- 2026-11-24: Added headless mGBA runner and source-based write counters for mode (`0x09FFFFFE`), CMD (`0x09800000`), and SD data (`0x09000000`, `0x0B000000`, `0x09200000`) registers. Clean Lite Build769 runs for 1/2/10/60 frames produced mode-write counts 32/312/2556/3060, then no more writes by frame 60; final state is mode `0x0005` (SDRAM+write, SD hidden), with zero decoded CMD/data writes. mGBA's default logger had produced 195,322 lines of unimplemented cartridge writes; runner now uses a silent logger and reports counters. This shows the current run does not reach SD initialization under the incomplete model; inspect post-startup PC and required ROM mapping before interpreting it as a kernel defect.
- 2026-11-24: Rechecked the DSSMM releases page; the current tags/commits still match all six `harnesses/cards.json` profiles. Original OMEGA Test 5 and BLUE Test 5 release notes explicitly say hardware-untested; SuperFW Build769 SD/Lite are also physically untested. Added source-derived SD idle-line reads and verified the real mGBA `load16` callback; model and core smoke pass. Clean Lite Build769 at 1,800 frames: 3,232 mode writes, mode `0x0007`, 144 CMD writes and zero DAT writes. `supercard_io.S` sends CMD0 then response code waits for CMD bit0 low; static idle-high cannot emulate the response. No card response or physical timing is claimed.
- 2026-11-24: Added a 64-halfword CMD write capture to the mGBA diagnostic runner. Repeated clean Lite Build769 run reproduces 3,232 mode writes, 144 CMD writes, zero DAT writes. Captured bits decode to CMD0 (`0x40 ... CRC 0x95`) followed by CMD8 (`0x48`), matching DSSMM's source routine that shifts each command byte left after every register write. This is source/runner validation only; the harness still has no SD response stream. Next host-test the decoder and create a small SD card protocol model before returning a response stream to firmware.
- 2026-11-24: Added and host-tested `supercard_sd_decode_command`, which reconstructs the six SD command bytes from 48 halfword bus writes using the observed bit-7 serial output. A generated CMD0 write sequence decodes exactly to `40 00 00 00 00 95`; malformed 47-write input is rejected. Model, mGBA smoke, runner build and `git diff --check` pass. This validates framing only; no SD command response model is present.
- 2026-11-24: Implemented synthetic native CMD8 R7 bit framing. Initial byte-aligned response failed to match DSSMM's assembly, which consumes seven bits for its first response byte; corrected the model to emit start, transmission/index, payload, CRC/end bits, then idle. Host model and mGBA smoke pass; clean Lite Build769 reaches CMD55 after reading CMD8 (`decoded_commands=3`, `responses=1`, 49 modeled response-line reads, final command 55). CMD55 still has no response, so SD init exits; this is not physical SD validation.
- 2026-11-24: Added synthetic SD responses for CMD55/ACMD41, CMD2/CID, CMD3/RCA, CMD9/CSD, CMD7, ACMD6 and CMD16 based on SuperFW's source expectations and native response framing. Host model and mGBA smoke pass. Clean Lite Build769 at both 1,800 and 2,400 frames reaches CMD18 then CMD12 (13 decoded commands, 10 responses, 666 response-line bits); block data remains absent and `sd_data_writes=0`. This shows init gets through command setup, but does not prove physical SD correctness. Next model Lite's 32-bit read port and back it with a configurable virtual SD image.
