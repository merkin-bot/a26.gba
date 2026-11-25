# Crash Recovery

- **Goal:** Finish a hardware-evidence-based mGBA harness for all six DSSMM card/kernel profiles before changing the A26 plugin. Model GBA hardware, matching card/kernel behavior, and SD storage as separate layers. Plugin fixes belong in the plugin; do not make the harness compensate for them.
- **Current step:** The three exact-pinned EZ source lanes are audited in `docs/provenance/ezflash-source-audit.md`; all five launch fixture cases passed per lane. `ezflash-mgba-trace-smoke` proves four-word channel-3 DMA reads/writes pass through the passive callback observer; traced and untraced cores match transfer hashes and DMA scheduling/wait-cycle fields. A synthetic SD-image adapter boundary is documented with status/latency explicitly `UNVERIFIED`. No EZ card or SD image is emulated yet. SuperCard Build769 plugin execution and the Chis NOR/SRAM evidence gap are in `PROGRESS.md`/`harnesses/cards.json`. Continue harness software-path coverage; do not optimize the plugin until the harness gate is complete. Then resume Atari timing/audio work, prioritizing input response, stable game time and synchronized audio, targeting 30 rendered fps with frame drops allowed.
- **Known results:** Current EZ release tags each pass their five DSSMM launch fixture cases. SuperFW Build769 clean builds for Chis/SD/Lite each pass two native settings boots over fixture virtual SRAM; Chis also passes 12 menu scenes and four NOR-carousel scenes. These are not physical card or SD-bus qualification. Full details/hashes are in `PROGRESS.md` and `harnesses/cards.json`.
- **Last pushed commit:** `b01e3f1` (`Repair Codespaces harness environment setup`). Working tree is clean. The Codespace had been recreated and generated binaries/build directories were absent.
- **Workspace:** `/workspaces/a26.gba`. mGBA clone at `harnesses/third_party/mgba` (commit `431935616c0e216d650b25b2ea0c0acccd359038`, MPL-2.0); DSSMM clone at `harnesses/third_party/dssmm`, currently detached at SuperFW `superfw-build769` (`37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4`). Clones and build outputs are git-ignored.

## Resume after Codespace restart

1. `git status --short --branch && git log -5 --oneline`; read `AGENTS.md`, `HANDOFF.md`, and `PROGRESS.md`.
2. If the devcontainer was edited or required commands are absent, run **Dev Containers: Rebuild Container** (the config now installs Docker-in-Docker and the setup script checks Docker, mGBA, Xvfb, xdotool, Stella and FAT tools). The old image had no Docker feature and omitted `mgba-sdl`, `xvfb`, `xdotool`, `xauth`, `stella`, `dosfstools`, and `mtools`. A live recovery used `sudo apt-get update && sudo apt-get install -y mgba-sdl xvfb xdotool xauth stella dosfstools mtools`; all those tools then resolved. `source/atari2600/prototype.gba` is generated/ignored and must be rebuilt. Rebuilding via Docker requires a working daemon; manually installing `docker.io` inside the current nested Codespace did not work because `dockerd` lacks iptables permissions. The configured Docker-in-Docker feature supplies its own privileged daemon entrypoint per its feature metadata, but it requires a container rebuild and has not yet been validated in a rebuilt Codespace.
3. If source clones are absent, clone `https://github.com/voluptuary/dssmm` to `harnesses/third_party/dssmm` and `https://github.com/mgba-emu/mgba` to `harnesses/third_party/mgba`. Exact release tags/commits are in `harnesses/cards.json`.
4. Run the standalone model and mGBA integration checks:
   ```sh
   cmake -S harnesses -B build/harness-cmake -G Ninja
   cmake --build build/harness-cmake --target supercard-model-test mgba-core-smoke supercard-mgba-runner ezflash-mgba-trace-smoke
   build/harness-cmake/supercard-model-test
   build/harness-cmake/mgba-core-smoke
   build/harness-cmake/ezflash-mgba-trace-smoke
   bash scripts/test-baseline.sh
   ```
   Diagnostic usage: `A26_SUPERCARD_BOARD=lite|sd|chis build/harness-cmake/supercard-mgba-runner <kernel-gba> <frames> [sd-image [capture.ppm [key-events]]]`. Key events are comma-separated `frame:mask` values (A=1, Down=0x80). Recreate SD image with `bash scripts/verify-harness.sh`; current per-board results and the SD kernel stop point are in `PROGRESS.md`.
5. Before switching SuperFW `BOARD`, run `make clean`; the profiles share generated intermediate paths. Use `BOARD=chis`, `sd`, or `lite` and `DSSMM_BUILD_NUMBER=769`. Current native settings fixture requires host wrappers for `arm-none-eabi-nm`, `-objcopy`, and `-gdb` under `/tmp/a26-container-tools`.
6. Current-session recovery results: CMake harness targets built; `supercard-model-test`, `mgba-core-smoke`, and `ezflash-mgba-trace-smoke` passed. `scripts/verify-harness.sh` is currently blocked because the ignored/generated plugin image is absent. Docker startup was attempted after installing `docker.io`, but nested-container iptables permission was denied. Update this status after rebuilding the devcontainer and regenerating artifacts.

## Reproducible project checks

- Plugin build: `bash scripts/build-plugin.sh`
- Baseline fixture: `bash scripts/test-baseline.sh`
- Profile release/test state and exact commands: `PROGRESS.md`, `harnesses/cards.json`
- Never commit proprietary kernels, BIOS images, generated SD images or ignored third-party clones.
