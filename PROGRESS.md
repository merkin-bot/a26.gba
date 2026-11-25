# Project state and next actions

**Updated:** 2026-11-25 UTC
**Branch:** main
**Latest code checks:** run at `800fa94`; the following `fdaedc6` commit changed handoff text only. Check `git log` and the working tree on each session start.

This is the source of truth for current status, exact next work, evidence, and test outcomes. Project requirements are in `AGENTS.md`; session kickoff is in `HANDOFF.md`; environment setup is in `WORKSPACE.md`.

## 1. Current task and next action

**Current task:** Re-establish a clean, reproducible unrestricted Ubuntu 24.04 Codespaces workspace and verify the plugin and harness baseline.

**Next action:** In the new unrestricted Codespace, follow `WORKSPACE.md` from its first step. Record the installed devkitARM version, run the baseline and harness commands, and reconcile the resulting artifacts/results here. If an official installation endpoint fails, autonomously use devkitPro's current official Linux instructions and document what worked; do not reintroduce Docker or stop for owner input. Then resume harness qualification below.

**Why this comes first:** The current/restricted workspace has no devkitARM or `gbafix`; the plugin build, FAT image verification, and full baseline have not been rerun after replacing Docker with host tools. The host-side CMake model/core/EZ trace smokes and PogoShell ABI check pass. This qualification is outstanding, not a plugin failure.

## 2. End goal and required order

Acceptance target and engineering rules are defined in AGENTS.md. Do not optimize the plugin before the harness gate; validate emulator behavior against the evidence-backed hardware model.

### Ordered work

1. **Workspace/baseline:** complete the current task above.
2. **Harness gate:** finish evidence-backed mGBA paths for the six profiles in `harnesses/cards.json`. Cover, as separate layers, GBA hardware; each matching card/kernel/launch profile; and SD/block storage with kernel/plugin/test ROM. Reuse DSSMM fixtures rather than duplicate launcher/UI tests. Test launch, ROM handoff, saves/storage, and return where evidence supports them. Clearly label seams/mocks and unknowns. Never alter a hardware model to make the Atari plugin pass.
3. **Emulator timing/performance:** establish real GBA timing evidence and correct measured bottlenecks.
4. **TIA audio:** implement and measure synchronized sound without compromising game time/video or shell operation.
5. **Menu parity:** compare with Goomba and record exact matches/gaps.
6. **Compatibility:** bankswitching, PAL/NTSC, controls, settings, and saves.

## 3. Current repository and harness state

- Plugin source: `source/atari2600/`; experimental/reference binary: `dist/a26.gba`; open-source test ROM: `test-roms/flappy.a26`.
- All six card profiles, matching kernel references, and profile-specific qualification state: `harnesses/cards.json`.
- Harness layering, implemented seams, commands and limits: `harnesses/README.md` and `docs/provenance/`.
- Ignored clones expected by harness work: mGBA `431935616c0e216d650b25b2ea0c0acccd359038` (MPL-2.0); DSSMM SuperFW `37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4` (Apache-2.0). Restore/build them with `WORKSPACE.md` as needed.
- Generated plugin, fixture, SD images, build directories and third-party clones are ignored; never add them to Git.

### Verified harness results

- EZ DE, Original OMEGA, and BLUE source-pinned DSSMM native launch suites each pass five launch fixture cases. These mock storage/boot seams; they do not qualify physical card or SD behavior.
- EZ passive mGBA trace observes four-word channel-3 DMA reads and writes while forwarding upstream behavior. Traced/untraced hashes and DMA scheduling/wait-cycle fields match. This is an observer, not an EZ card or SD emulator.
- SuperFW Build769 Chis/SD/Lite clean builds and native settings fixtures passed; Chis menu/NOR scenes passed. Physical bus, SD, NOR, and board-revision behavior remains partly `UNVERIFIED`.
- The SuperCard mGBA diagnostic reaches A26 plugin code for all three Build769 board profiles when using the synthetic model and SD image. This does not prove physical-card fidelity, real GBA timing, correct game output, or audio.
- Detailed test commands, result hashes, known harness/hardware differences, and source citations remain in `harnesses/README.md`, `harnesses/cards.json`, and `docs/provenance/`.

### Unverified and incomplete

- Full EZ card + SD block-image integration and physical status/response timing.
- Chis NOR/SRAM model: available HDL and Build769 source differ; board revision correspondence is not established.
- Full hardware qualification for all six cards/kernels, including real saves and shell return.
- SuperCard Build 822 behavior beyond the owner's report that the plugin runs slowly and silently.
- Accurate native Atari game speed, input latency, smooth GBA frame pacing, and synchronized sound on real hardware. No valid measured baseline for those outcomes is recorded yet.

## 4. Latest checks and environment state

The current Ubuntu 24.04.5 host has the documented Ubuntu dependencies installed and the pinned ignored mGBA/DSSMM checkouts restored. The official bootstrap URL `https://apt.devkitpro.org/install-devkitpro-pacman` returned HTTP 403. The official devkitPro pacman GitHub release notes point Debian/Ubuntu users back to that same bootstrap script; the Linux/macOS release asset is an installer package, not documented as an Ubuntu bootstrap alternative. No devkitARM or `gbafix` was installed, so do not infer plugin build status or hash.

Current host checks:

- PASS: `python3 scripts/test-pogo-abi.py`
- PASS: `cmake -S harnesses -B build/harness-cmake -G Ninja`
- PASS: `cmake --build build/harness-cmake --target supercard-model-test mgba-core-smoke supercard-mgba-runner ezflash-mgba-trace-smoke -j2`
- PASS: `build/harness-cmake/supercard-model-test`
- PASS: `build/harness-cmake/mgba-core-smoke`
- PASS: `build/harness-cmake/ezflash-mgba-trace-smoke`
- BLOCKED by absent devkitARM/general-tools: `bash scripts/build-plugin.sh`, `bash scripts/build-fixture.sh`, `bash scripts/test-baseline.sh`; each exits with the workspace prerequisite message.
- BLOCKED because the plugin binary cannot be generated: `bash scripts/verify-harness.sh` (`source/atari2600/prototype.gba: No such file or directory`).
- Tool check: `arm-none-eabi-gcc --version` reports command not found.

The following code checks last ran at `800fa94`; `fdaedc6` changed documentation only:

- `cmake -S harnesses -B build/harness-cmake -G Ninja`
- `cmake --build build/harness-cmake --target supercard-model-test mgba-core-smoke supercard-mgba-runner ezflash-mgba-trace-smoke -j2`
- `build/harness-cmake/supercard-model-test`
- `build/harness-cmake/mgba-core-smoke`
- `build/harness-cmake/ezflash-mgba-trace-smoke`
- `python3 scripts/test-pogo-abi.py`
- `git diff --check` and shell syntax checks for the changed build scripts

Not run in that workspace: `bash scripts/build-plugin.sh`, `bash scripts/verify-harness.sh`, and `bash scripts/test-baseline.sh`. `arm-none-eabi-gcc` and `gbafix` were absent and `source/atari2600/prototype.gba` was not generated. Run these in the new workspace; do not carry the old PASS status forward without rerunning.

The previous devcontainer/Docker-in-Docker setup failed because the restricted Codespace could not start nested Docker (iptables/NAT permission denied). `.devcontainer/` was removed. All build scripts now use installed host devkitARM tools. `WORKSPACE.md` is authoritative; do not restore Docker-in-Docker.

## 5. Durable constraints

Follow the hardware fidelity, evidence, measurement, build, mGBA, board-switching, and owner code-use rules in `AGENTS.md`. Profile-specific unknowns and evidence are tracked in `harnesses/cards.json` and `docs/provenance/`.

## 6. Session record

- 2026-11-25: Session kickoff: read AGENTS.md, HANDOFF.md, PROGRESS.md, and WORKSPACE.md in order; clean `main` matches `origin/main` at `1ec9fde`. Current host is Ubuntu 24.04.5, but devkitARM/gbafix and the documented baseline tools are absent. Workspace qualification started; see follow-up entry for install/build outcome.
- 2026-11-25: Installed the documented Ubuntu host dependencies with apt and restored mGBA `431935616c0e216d650b25b2ea0c0acccd359038` plus DSSMM `37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4` as ignored checkouts. CMake harness targets and three host smoke executables plus PogoShell ABI check pass. Plugin/full baseline remains blocked because official devkitPro Debian bootstrap returns HTTP 403; official GitHub release notes refer Debian users to the same script and do not document an Ubuntu alternative. No plugin optimization or hardware claim was made.
- 2026-11-25: Removed the failed devcontainer/Docker-in-Docker path. Build scripts use host devkitARM; `WORKSPACE.md` describes fresh unrestricted Codespaces setup. Host-side model/core/EZ trace smokes and ABI check pass. Plugin/FAT/full-baseline checks await a new workspace with devkitARM installed.
- 2026-11-25: Owner clarified the final quality bar: accurate native Atari speed, smooth frame pacing and synchronized sound comparable in feel to established GBC/NES/SMS emulators. Hardware-faithful harnesses must never be changed to accommodate the emulator. Owner authorizes GitHub repository code use under GPL-2.0 without separate permission requests.
- 2026-11-25: Consolidated cold-start roles across AGENTS.md, HANDOFF.md, PROGRESS.md, and WORKSPACE.md. No code or harness behavior changed.
