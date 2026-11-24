# Crash Recovery

- **Goal:** Finish a hardware-evidence-based mGBA harness for all six DSSMM card/kernel profiles before changing the A26 plugin. Model GBA hardware, matching card/kernel behavior, and SD storage as separate layers. Plugin fixes belong in the plugin; do not make the harness compensate for them.
- **Current step:** SD writes remain synthetic, source-shaped Lite Build769 behavior; physical protocol is UNVERIFIED. Implemented mGBA instruction fetch mapping from SuperCard mode: firmware mode uses original kernel ROM, SDRAM mode uses image-backed model RAM, and detach restores mGBA's allocation. Core smoke, model test, plugin build, and image verification pass. Two fresh selected-ROM runs are identical: 2,000 frames, PC `0x0800788A` (`decRead`), 850 SD commands, 387 reads, 13 writes, zero write failures, 240x160 hash `E6AFE00D3895221D`. The capture has 8 colors and visible game imagery. This proves the plugin executes under mGBA HLE plus the synthetic SuperCard model; it does not establish real hardware behavior, correct game state, synchronized Stella output, audio, or GBA timing. Next: source-audit SuperFW Chis/SD fetch mapping independently; then cover each EZ profile. Keep plugin untouched. Stella's current 240x160 capture SHA-256 is `912c4cc2e7e45b8d9907a83717479e9c27fad45bced8499c34864d64f138fc46`; state is not synchronized. Input response, stable native time, and audio sync remain priorities, targeting 30 rendered fps with drops allowed after all six card paths qualify.
- **Known results:** Current EZ release tags each pass their five DSSMM launch fixture cases. SuperFW Build769 clean builds for Chis/SD/Lite each pass two native settings boots over fixture virtual SRAM; Chis also passes 12 menu scenes and four NOR-carousel scenes. These are not physical card or SD-bus qualification. Full details/hashes are in `PROGRESS.md` and `harnesses/cards.json`.
- **Last pushed commit:** `8ddf96f` (`Trace reset abort stack and frame window`); SuperCard instruction-fetch mapping and current findings are uncommitted.
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
