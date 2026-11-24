# Crash Recovery

- **Goal:** Finish a hardware-evidence-based mGBA harness for all six DSSMM card/kernel profiles before changing the A26 plugin. Model GBA hardware, matching card/kernel behavior, and SD storage as separate layers. Plugin fixes belong in the plugin; do not make the harness compensate for them.
- **Current step:** Board profiles are implemented in the uncommitted worktree. Model/core tests pass. With matching clean Build769 kernels and fresh separate images, Lite and Chis selected-ROM runs reach `0x0800788A` (`decRead`) at 2,000 frames, hash `E6AFE00D3895221D`, 850 SD commands, 387 reads, 13 writes, zero failures. The SD profile stops in kernel at `0x027CC548` after 13 commands and 520 data-port values. Host tests cover SD read aliases, serial writes, and Chis NOR register writes; physical behavior, Chis NOR backing and SRAM banking remain UNVERIFIED. **Active:** diagnose SD kernel's early stop, then implement only source-evidenced card behavior. Do not change plugin code or GBA/card rules to force handoff. Stella 240x160 hash `912c4cc2e7e45b8d9907a83717479e9c27fad45bced8499c34864d64f138fc46` is unsynchronized. Input response, stable native time, and audio sync remain priorities, target 30 rendered fps with frame drops allowed.
- **Known results:** Current EZ release tags each pass their five DSSMM launch fixture cases. SuperFW Build769 clean builds for Chis/SD/Lite each pass two native settings boots over fixture virtual SRAM; Chis also passes 12 menu scenes and four NOR-carousel scenes. These are not physical card or SD-bus qualification. Full details/hashes are in `PROGRESS.md` and `harnesses/cards.json`.
- **Last pushed commit:** `021ef81` (`Document SuperFW board-specific I/O`). Board-profile implementation and this updated handoff are currently uncommitted.
- **Workspace:** `/workspaces/a26.gba`. mGBA clone at `harnesses/third_party/mgba` (commit `431935616c0e216d650b25b2ea0c0acccd359038`, MPL-2.0); DSSMM clone at `harnesses/third_party/dssmm`, currently detached at SuperFW `superfw-build769` (`37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4`). Clones and build outputs are git-ignored.

## Resume after Codespace restart

1. `git status --short --branch && git log -5 --oneline`; read `AGENTS.md`, `HANDOFF.md`, and `PROGRESS.md`.
2. Check `docker`, `/usr/games/mgba`, `xvfb-run`, and `xdotool`. Recreate session-local Docker wrappers from `PROGRESS.md` if `/tmp/a26-make-container/make` or `/tmp/a26-container-tools/arm-none-eabi-*` are absent. Host mGBA is 0.10.2; DSSMM fixture JSON currently says 0.10.1.
3. If source clones are absent, clone `https://github.com/voluptuary/dssmm` to `harnesses/third_party/dssmm` and `https://github.com/mgba-emu/mgba` to `harnesses/third_party/mgba`. Exact release tags/commits are in `harnesses/cards.json`.
4. Run the standalone model and mGBA integration checks:
   ```sh
   cmake -S harnesses -B build/harness-cmake -G Ninja
   cmake --build build/harness-cmake --target supercard-model-test mgba-core-smoke supercard-mgba-runner
   build/harness-cmake/supercard-model-test
   build/harness-cmake/mgba-core-smoke
   bash scripts/verify-harness.sh
   ```
   Diagnostic usage: `A26_SUPERCARD_BOARD=lite|sd|chis build/harness-cmake/supercard-mgba-runner <kernel-gba> <frames> [sd-image [capture.ppm [key-events]]]`. Key events are comma-separated `frame:mask` values (A=1, Down=0x80). Recreate SD image with `bash scripts/verify-harness.sh`; current per-board results and the SD kernel stop point are in `PROGRESS.md`.
5. Before switching SuperFW `BOARD`, run `make clean`; the profiles share generated intermediate paths. Use `BOARD=chis`, `sd`, or `lite` and `DSSMM_BUILD_NUMBER=769`. Current native settings fixture requires host wrappers for `arm-none-eabi-nm`, `-objcopy`, and `-gdb` under `/tmp/a26-container-tools`.
6. Do not claim full harness success or resume plugin optimization until actual card/SD launch paths are covered. Lite/Chis selected-ROM paths run in the synthetic model; SuperCard SD currently stops during early startup. SD responses/timing and physical behavior are not qualified.

## Reproducible project checks

- Plugin build: `bash scripts/build-plugin.sh`
- Baseline fixture: `bash scripts/test-baseline.sh`
- Profile release/test state and exact commands: `PROGRESS.md`, `harnesses/cards.json`
- Never commit proprietary kernels, BIOS images, generated SD images or ignored third-party clones.
