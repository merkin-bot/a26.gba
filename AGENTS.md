# Instructions for repository agents

If the user says “Read AGENTS.md,” treat that as the task kickoff. Read this file, then `HANDOFF.md`, `PROGRESS.md`, and `WORKSPACE.md` in that order. Check `git status --short --branch` and `git log -5 --oneline`. Continue autonomously from the current task; do not ask the owner to repeat repository state or approve ordinary implementation decisions.

## Project outcome

Deliver a usable Atari 2600 emulator as a GBA PogoShell plugin for the supported EZ-FLASH Omega and SuperCard families. The emulator must run games at accurate native Atari speed, respond promptly to input, render smoothly, and produce TIA sound synchronized with emulated game time. The experience should approach established GBC, NES, and SMS emulators within GBA hardware limits. Smooth 30 rendered fps is a working target; preserve native emulated time and audio even when frames must be dropped. Do not present the target as achieved until measured and verified.

The current plugin runs on an unpublished SuperCard Build 822 trial but is reported slow and silent. Menu/font parity is incomplete. The exact current code and test state is in `PROGRESS.md`.

## Priority order

1. Build and document a reproducible unrestricted Codespaces workspace using `WORKSPACE.md`; establish a clean plugin and harness baseline.
2. Complete reliable mGBA harness paths for the six card/kernel/storage profiles in `harnesses/cards.json`. Reuse DSSMM fixtures and avoid duplicate launcher tests. Clearly distinguish GBA hardware, card/kernel, and SD/block storage layers; identify mocks and mark unknown card behavior `UNVERIFIED`.
3. Only after the harness gate, measure and improve input response and Atari game-time accuracy. Measure before and after every emulation change. Inspect CPU/TIA scheduling, ROM waitstates, IWRAM/EWRAM placement, ARM/Thumb, prefetch, dispatch, rendering, and DMA. Desktop timing is not GBA evidence.
4. Implement and validate TIA audio through GBA Direct Sound and/or PSG, synchronized with the emulated clock without breaking video timing or shell behavior.
5. Compare menu behavior/font/layout/options/save-state conventions with PocketNES and Goomba where feasible; document exact comparisons and gaps. PocketNES is the primary interaction/style reference: keep its familiar menu structure, visual treatment, font, and hotkeys, adapting labels and controls to Atari 2600 options. Do not claim parity until each behavior has been compared.
6. Expand cartridge bankswitching, PAL/NTSC, controls, per-game settings, save support, and polish after the above is stable.

## Non-negotiable engineering rules

- **Hardware fidelity:** never change the GBA/card harness to accommodate an Atari emulator defect. Real hardware behavior outranks mGBA. The harness must reproduce evidence-backed hardware behavior, including behavior inconvenient to the emulator. Keep GBA, each card/kernel, and each SD image as distinct layers. Do not claim a harness mock proves physical hardware.
- **Evidence:** do not guess about PogoShell, EZ-FLASH, SuperCard, or timings. Inspect primary source/docs and the references in `reference/repositories.txt`, or design a safe experiment. Record evidence and limits in `PROGRESS.md` and relevant files under `docs/provenance/`. Mark unknowns `UNVERIFIED`.
- **Measurement:** no performance claim without repeatable before/after measurements. mGBA and desktop Stella are diagnostic/visual references, not real GBA timing evidence. Synchronize game state before comparing frames.
- **Atari image comparisons:** use the user's native-image guidance: 160x190 Atari output, with a 40x192 TIA playfield, mapped to the GBA's 240x160 framebuffer. Keep the playfield's 40-bit horizontal structure (four Atari pixels per bit) intact when scaling horizontally; without a side crop each bit maps to six GBA pixels. Preserve all active playfield scanlines when choosing the vertical crop/sample. CRT overscan typically masks about 10–12 Atari pixels at each side; Stella capture defaults to a 12-pixel crop per side and accepts an explicit per-ROM crop (including 0). Check each game's edges first because scores and gameplay can use them, and record the selected crop with comparisons. Do not scale the entire Stella desktop window. The reproducible transform is documented in `WORKSPACE.md` and `scripts/capture-stella-reference.sh`.
- **Additional homebrew ROMs:** `reference/repositories.txt` includes the RetroBrew Atari 2600 game catalog. Its README limits redistribution to its own site/project and per-game rights notices vary; do not treat all binaries as open source or commit/redistribute them. Stage a locally licensed/authorized test image by copying `.bin` to `.a26` with `scripts/stage-homebrew-rom.sh`; keep the result under ignored `work/` and inspect the matching `.txt` metadata.
- **Build hygiene:** make small verified changes. Run relevant checks; record failures and commands. Commit and push each completed step; never rewrite shared history. Leave the repository buildable or record precisely why it is not.
- **SuperFW boards:** `chis`, `sd`, and `lite` share generated filenames. Run `make clean` before switching `BOARD` and verify the corresponding `SUPERCHIS_IO`, `SUPERCARD_FLASH_ADDRPERM`, or `SUPERCARD_LITE_IO` define.
- **mGBA ABI:** static-core consumers inherit `$<TARGET_PROPERTY:mgba,COMPILE_DEFINITIONS>`; `struct mCore` has conditional fields. Card hooks must preserve upstream callback timing/results and cover `ARMCore.memory.loadMultiple`/`storeMultiple` for ARM LDM/STM, not just scalar reads/writes.
- **Owner's code-use authorization:** the owner directs that code from any GitHub repository may be used in this project under GPL-2.0 without seeking additional permission or licenses. Do not pause to request repository access/permission for such code. Keep source attribution and provenance when adapting code, and retain project notices. This authorization concerns repository code; it does not authorize redistributing commercial ROMs, copyrighted BIOS files, or proprietary kernels/card firmware. Use provided open-source homebrew for tests.
- **Autonomy:** do not pause for routine clarification or permission. Resolve choices using repository evidence and engineering judgment; continue independent work if a true hardware-only fact is unavailable, clearly record the blocker, and proceed with other tasks. Ask the owner only when progress truly depends on an observation or decision they alone can provide.

## Cold-start and workspace contract

`HANDOFF.md` tells you what to do first in this session. `PROGRESS.md` is the source of truth for current objectives, exact state, evidence, and test results. `WORKSPACE.md` is the single source of truth for installing tools, restoring ignored clones, building, and running checks in a fresh unrestricted Ubuntu 24.04 Codespace. Do not reintroduce `.devcontainer` or Docker-in-Docker; nested Docker failed in restricted Codespaces. Build scripts use host devkitARM.

At each session start, verify repository state against the handoff and correct stale documentation before other work. Before and after each task, update `PROGRESS.md`. At session end, ensure generated artifacts stay ignored, run relevant checks, update the handoff, commit, and push.

## Repository constraints

- Canonical plugin source: `source/atari2600/`; prepared binary/reference artifacts: `dist/`; open-source test ROM: `test-roms/`.
- Harness profiles/evidence: `harnesses/cards.json`, `harnesses/README.md`, and `docs/provenance/`.
- Generated plugin/kernel binaries, SD images, BIOS files, ROMs, and build outputs are not committed. Exact-pinned source snapshots and licensed offline bootstrap inputs may live under `vendor/offline/`; document their upstream revision, license/provenance, hashes, and purpose. Never add proprietary firmware or copyrighted ROMs to these bundles.
