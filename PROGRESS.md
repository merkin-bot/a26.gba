# PROGRESS.md

Handoff document for any AI agent or human continuing this project. Read `AGENTS.md` first, then this file in full.

**Last updated:** 2026-11-24 UTC by Codex
**Current branch:** main
**Last pushed commit:** 85f8141

> Rules: update this file before starting each task and right after finishing each step. Commit and push it often. Delete stale info rather than piling up contradictions. Mark anything not confirmed as `UNVERIFIED`.

---

## 1. Project summary

- **Goal:** Atari 2600 emulator for the Game Boy Advance, running as a PogoShell plugin on EZ-FLASH Omega family and SuperCard family devices. Continues https://github.com/voluptuary/dssmm.
- **Status:** Runs on unpublished SuperCard build 822, but slowly, with no sound, and menu parity with other PogoShell plugins (e.g. Goomba) is incomplete (different menu font). Existing source archive has a prepared GPLv3 source build, an experimental plugin binary, an open-source generated diagnostic, and runtime checks from stock mGBA 0.10.1.
- **Current focus:** Repository cleanup and organization, while continuing the flashcard harness baseline.

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
- **Task:** Performance optimization with Stella desktop reference baseline.
- **Exact step:** DE and Original OMEGA DSSMM native mGBA launch suites pass all five cases each; BLUE suite is running after moving its stale nested target build directory. All traces stop at mocked card handoff and do not verify full SD/card I/O. Actual mGBA runtime is 0.10.2; DSSMM generated JSON hardcodes stale 0.10.1. Next check SuperFW Chis/SD/Lite build and native mGBA preview coverage, then design only the uncovered GBA/card/SD layer.
- **Files touched:** `AGENTS.md`, `HANDOFF.md`, `PROGRESS.md`, `harnesses/README.md`, `harnesses/cards.json`, `harnesses/.gitignore`.
- **Uncommitted changes:** None after commit `3c71a24`; test outputs and DSSMM clone are ignored.
- **Branch:** main
- **Resume command:** `cd harnesses/third_party/dssmm && PATH=/tmp/a26-make-container:$PATH MGBA_BIN=/usr/games/mgba bash ezfomegade/scripts/test-native-launch-mgba.sh` (repeat for `ezfomega` and `ezfblue`; wrapper is session-local).

### Next up (ordered)
1. Reuse and qualify existing DSSMM mGBA fixtures for all six matching kernels: three EZ-FLASH profiles and SuperFW Chis/SD/Lite.
2. Implement only the missing mGBA hardware/card/SD layers needed to run the actual kernel and its FAT-based plugin/ROM loading path.
3. Keep mGBA harness behavior separate from plugin workarounds; document hardware gaps.
4. After harness verification, resume plugin timing/optimization with responsive input and stable 30 fps plus audio sync as targets.
5. Compare Stella and plugin output at synchronized game states; hardware qualification remains final authority.

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
| SuperCard mode/SDRAM host model | `build/harness/supercard-model-test` | PASS | Magic/register mode and SDRAM payload copy pass strict C11 tests |
| EZ-FLASH Omega card harness | `<TODO>` | PARTIAL | Omega source exists; kernel launch and card register fixture remain |
| SuperCard 822 card harness | `<TODO>` | PARTIAL | DSSMM SuperFW source provides portable ABI and SuperCard boundaries; card protocol evidence still limited |
| Minimal mGBA SuperCard runner | `build/harness/supercard-mgba-runner` | BLOCKED | Scratch static-core crashes at `core->init`; hook code not yet qualified |

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
- Minimal copied mGBA static-core runner crashes at `core->init(core)` even before ROM load. Retested through an EZ-FLASH-style CMake build; both scratch-linked variants crash, so the failure is in static-core platform setup, not BIOS or card hooks. Runner removed; host model remains passing.

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
