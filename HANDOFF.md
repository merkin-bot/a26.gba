# Crash Recovery

- **Goal:** Finish a hardware-evidence-based mGBA harness for all six DSSMM card/kernel profiles before changing the A26 plugin. Model GBA hardware, matching card/kernel behavior, and SD storage as separate layers. Plugin fixes belong in the plugin; do not make the harness compensate for them.
- **Current step:** ACMD23/CMD25 and Lite STM write path are implemented and unit-tested; SD protocol/timing is synthetic and physical behavior is UNVERIFIED. Model/core-hook, plugin build, and FAT32 image verification pass. Reproduce selected-ROM trace with `build/harness-cmake/supercard-mgba-runner harnesses/third_party/dssmm/superfw/build/release-builds/superfw-build769-verified/lite.gba 2000 work/a26-sd.img work/supercard-selection.ppm 1650:0x1,1700:0,1750:0x1,1800:0,1850:0x1,1900:0`: 850 commands, 13 persisted writes, 387 reads, staged plugin header at SDRAM, PC=`0x000003D4`, LR=`0x08000000`, frame hash `6A7C6786F2131325`. At 3,000 frames PC=`0x00000008`, writes=302, same hash; plugin gameplay is not proven. DSSMM reset tail copies plugin to EWRAM, sets IWRAM `0x03007FFA=1`, and calls BIOS SoftReset; mGBA HLE then selects EWRAM address `0x02000000`. Runner prints post-run SoftReset flag and EWRAM/SDRAM entry bytes. Fresh-image trace: flag=0, EWRAM entry is zero, SDRAM entry matches plugin. Because this is only the final state, it does not show whether plugin code ran earlier. The prior 3,000-frame run reused a modified image; discard it as an independent comparison. Recreate the image for every run and next trace the SoftReset boundary/PC-region transitions without changing GBA/card/SD behavior. Keep the three layers separate: GBA hardware, each card plus matching kernel, and that card SD image. Stella Flappy output is scaled to 240x160 for later visual comparison; states are not synchronized. Once all six card paths qualify, prioritize responsive input and stable native game time/audio, target 30 rendered fps with frame drops allowed. No physical card qualification.
- **Known results:** Current EZ release tags each pass their five DSSMM launch fixture cases. SuperFW Build769 clean builds for Chis/SD/Lite each pass two native settings boots over fixture virtual SRAM; Chis also passes 12 menu scenes and four NOR-carousel scenes. These are not physical card or SD-bus qualification. Full details/hashes are in `PROGRESS.md` and `harnesses/cards.json`.
- **Last pushed commit:** `7890517` (`Model Lite SuperCard SD write path`).
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
   Diagnostic usage: `build/harness-cmake/supercard-mgba-runner <kernel-gba> <frames> [sd-image [capture.ppm [key-events]]]`. Key events are comma-separated `frame:mask` values (A=1, Down=0x80). Recreate SD image with `bash scripts/verify-harness.sh`; exact Lite Build769 browser command and latest frame hashes are in `PROGRESS.md`.
5. Before switching SuperFW `BOARD`, run `make clean`; the profiles share generated intermediate paths. Use `BOARD=chis`, `sd`, or `lite` and `DSSMM_BUILD_NUMBER=769`. Current native settings fixture requires host wrappers for `arm-none-eabi-nm`, `-objcopy`, and `-gdb` under `/tmp/a26-container-tools`.
6. Do not claim full harness success or resume plugin optimization until actual card/SD launch paths are covered. The runner models synthetic SD command responses and Lite image-backed reads, but not complete card protocol/timing or physical behavior.

## Reproducible project checks

- Plugin build: `bash scripts/build-plugin.sh`
- Baseline fixture: `bash scripts/test-baseline.sh`
- Profile release/test state and exact commands: `PROGRESS.md`, `harnesses/cards.json`
- Never commit proprietary kernels, BIOS images, generated SD images or ignored third-party clones.
