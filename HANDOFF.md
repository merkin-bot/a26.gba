# Crash Recovery

- **Goal:** Finish a hardware-evidence-based mGBA harness for all six DSSMM card/kernel profiles before changing the A26 plugin. Model GBA hardware, matching card/kernel behavior, and SD storage as separate layers. Plugin fixes belong in the plugin; do not make the harness compensate for them.
- **Current step:** Release tags were rechecked and match `harnesses/cards.json`: DE `v8.4.1`, Original OMEGA Test 5, BLUE Test 5, SuperFW Build769. Source-derived idle-high CMD/DAT0 reads pass model and real mGBA `load16` smoke checks. Clean Lite Build769 runner at 1,800 frames reports 3,232 mode writes, mode `0x0007`, 144 CMD writes, zero DAT writes. First 64 CMD writes begin `40 80 00 ...`, include CMD0 with CRC byte `95`, and then CMD8 (`48`), matching DSSMM's byte-shift loop. Next decode and host-test command framing, then implement a minimal SD protocol card model with capacity/image backing. No physical card timing, PogoShell launch or Atari ROM path is integrated.
- **Known results:** Current EZ release tags each pass their five DSSMM launch fixture cases. SuperFW Build769 clean builds for Chis/SD/Lite each pass two native settings boots over fixture virtual SRAM; Chis also passes 12 menu scenes and four NOR-carousel scenes. These are not physical card or SD-bus qualification. Full details/hashes are in `PROGRESS.md` and `harnesses/cards.json`.
- **Last pushed commit:** `966a1b6` (headless SuperCard mGBA diagnostic runner); current working changes are not committed yet. Use `git log -1 --oneline` for repository state.
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
   ```
   Diagnostic usage: `build/harness-cmake/supercard-mgba-runner <local-gba-image> <frames>`. It counts frames and mode/CMD/DAT writes. It supplies documented idle-high line values but no SD command responses, cartridge ROM mapping, or block transfers. A clean Lite Build769 image reached frame 1,800 and sent CMD0; see `PROGRESS.md` for counts and limits.
5. Before switching SuperFW `BOARD`, run `make clean`; the profiles share generated intermediate paths. Use `BOARD=chis`, `sd`, or `lite` and `DSSMM_BUILD_NUMBER=769`. Current native settings fixture requires host wrappers for `arm-none-eabi-nm`, `-objcopy`, and `-gdb` under `/tmp/a26-container-tools`.
6. Do not claim full harness success or resume plugin optimization until actual card/SD launch paths are covered. The headless runner does not emulate SD commands yet.

## Reproducible project checks

- Plugin build: `bash scripts/build-plugin.sh`
- Baseline fixture: `bash scripts/test-baseline.sh`
- Profile release/test state and exact commands: `PROGRESS.md`, `harnesses/cards.json`
- Never commit proprietary kernels, BIOS images, generated SD images or ignored third-party clones.
