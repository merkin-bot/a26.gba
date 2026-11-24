# Crash Recovery

- **Goal:** Finish a hardware-evidence-based mGBA harness for all six DSSMM card/kernel profiles before changing the A26 plugin. Model GBA hardware, matching card/kernel behavior, and SD storage as separate layers. Plugin fixes belong in the plugin; do not make the harness compensate for them.
- **Current step:** `ce005a2` records the Chis Build769/CPLD mismatch; NOR/SRAM mapping remains unimplemented and UNVERIFIED. All three clean Build769 SuperCard kernels reach plugin code in synthetic mGBA (Lite/Chis `0x0800788A`, hash `E6AFE00D3895221D`; SD `pl_normal` at `0x080062A2`, hash `C321C3B99B2E799D`). **Active:** audit the three EZ-FLASH kernels' own cartridge/SD/save access paths from their pinned source and reuse the existing native launch fixtures. Do not reuse SuperCard hardware assumptions. Keep plugin unchanged. Stella comparison remains unsynchronized. Input response, stable native time and audio sync remain priorities; target 30 rendered fps with frame drops allowed.
- **Known results:** Current EZ release tags each pass their five DSSMM launch fixture cases. SuperFW Build769 clean builds for Chis/SD/Lite each pass two native settings boots over fixture virtual SRAM; Chis also passes 12 menu scenes and four NOR-carousel scenes. These are not physical card or SD-bus qualification. Full details/hashes are in `PROGRESS.md` and `harnesses/cards.json`.
- **Last pushed commit:** `ce005a2` (`Document unresolved SuperChis mapping evidence`). Workspace was clean after push.
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
6. Do not claim full harness success or resume plugin optimization until all six card/SD paths are covered. All three SuperCard selected-ROM paths reach plugin code in the synthetic model; EZ profiles remain. SD responses/timing and physical behavior are not qualified.

## Reproducible project checks

- Plugin build: `bash scripts/build-plugin.sh`
- Baseline fixture: `bash scripts/test-baseline.sh`
- Profile release/test state and exact commands: `PROGRESS.md`, `harnesses/cards.json`
- Never commit proprietary kernels, BIOS images, generated SD images or ignored third-party clones.
