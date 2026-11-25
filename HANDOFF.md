# Crash Recovery

- **Goal:** Finish hardware-evidence-based mGBA harness paths for all six DSSMM card/kernel profiles before changing the A26 plugin. Keep GBA hardware, card/kernel behavior, and SD storage as separate layers. Plugin fixes belong in the plugin, not the harness.
- **Current step:** The EZ source lanes are audited and their five launch fixtures pass per lane. EZ passive trace smoke observes four-word channel-3 DMA reads/writes while preserving upstream transfer hashes and scheduling/wait-cycle fields. This is an observer only; no EZ card/SD response is emulated. SuperCard Build769 plugin execution and the Chis NOR/SRAM evidence gap remain documented in PROGRESS.md and harnesses/cards.json. Continue harness software-path coverage; do not optimize the plugin until the harness gate is complete.
- **Known results:** Current EZ release tags pass five DSSMM launch fixture cases each. SuperFW Build769 clean builds for Chis/SD/Lite each pass two native settings boots over fixture virtual SRAM; Chis also passes 12 menu scenes and four NOR-carousel scenes. These are not physical card or SD-bus qualification.
- **Last known commit:** See git log; this document and WORKSPACE.md are updated with the latest committed setup.
- **Workspace:** /workspaces/a26.gba. mGBA clone at harnesses/third_party/mgba (commit 431935616c0e216d650b25b2ea0c0acccd359038, MPL-2.0); DSSMM clone at harnesses/third_party/dssmm (SuperFW commit 37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4, Apache-2.0). Clones/build outputs are ignored.

## Resume in the new Codespace

1. Start a regular Ubuntu 24.04 Codespace using the unrestricted host environment. This repository intentionally has no .devcontainer and must not use Docker-in-Docker; nested Docker failed in the restricted Codespace with iptables/NAT permission denial.
2. Read AGENTS.md, HANDOFF.md, PROGRESS.md, and WORKSPACE.md. Run git status --short --branch and git log -5 --oneline.
3. Follow WORKSPACE.md to install host packages and official devkitPro/devkitARM, restore pinned ignored clones, and run the build/baseline checks. The devkitPro bootstrap previously returned HTTP 403 in restricted mode; verify it in the unrestricted Codespace and record exact compiler version/hash. The current Codespace has no arm-none-eabi-gcc or gbafix, so plugin build and FAT verification were not run here.
4. Run the standalone checks:

       cmake -S harnesses -B build/harness-cmake -G Ninja
       cmake --build build/harness-cmake --target supercard-model-test mgba-core-smoke supercard-mgba-runner ezflash-mgba-trace-smoke -j2
       build/harness-cmake/supercard-model-test
       build/harness-cmake/mgba-core-smoke
       build/harness-cmake/ezflash-mgba-trace-smoke
       bash scripts/test-baseline.sh

   Diagnostic usage: A26_SUPERCARD_BOARD=lite|sd|chis build/harness-cmake/supercard-mgba-runner <kernel-gba> <frames> [sd-image [capture.ppm [key-events]]]. Key events are comma-separated frame:mask values (A=1, Down=0x80). Recreate SD image with bash scripts/verify-harness.sh.
5. Before switching SuperFW BOARD, run make clean. Use BOARD=chis, sd, or lite and DSSMM_BUILD_NUMBER=769. Keep board-specific build and test state in harnesses/cards.json and PROGRESS.md.

## Current session verification

- Passed: CMake configure/build; supercard-model-test; mgba-core-smoke; ezflash-mgba-trace-smoke; PogoShell ABI/manifest check; shell syntax and git diff checks.
- Not run: plugin build, verify-harness.sh, and full baseline. The current Codespace lacks devkitARM/gbafix and source/atari2600/prototype.gba; these must be generated in the next unrestricted workspace.
- Host scripts build-plugin.sh, build-fixture.sh, and build-card-fixture.py have been changed to use host devkitARM; no Docker daemon is required.

## Reproducible project checks

- Full workspace setup and commands: WORKSPACE.md.
- Plugin build: bash scripts/build-plugin.sh
- Baseline: bash scripts/test-baseline.sh
- Profile release/test state and exact commands: PROGRESS.md and harnesses/cards.json.
- Never commit proprietary kernels, BIOS images, generated SD images, or ignored third-party clones.
