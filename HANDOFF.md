# Crash Recovery

- **Doing now:** Hardware faithful mGBA harness work for EZ-FLASH Omega and SuperCard. Do not resume plugin optimizations until the harness is verified; do not alter harness behavior to compensate for plugin problems.
- **Exact next step:** Published EZ release fixtures and all three clean SuperFW Build769 board settings fixtures pass, but none models the card SD bus. The host SuperCard register model is now corrected to the source-confirmed two-magic/two-mode-write protocol and builds through CMake. Continue the missing integrated mGBA GBA/card/SD path using DSSMM source evidence. Do not resume plugin tuning until harness is verified.
- **Branch:** `main`
- **Last known commit:** See `git log -1 --oneline`; previous remote commit was `ffe12d2`.
- **Build:** `bash scripts/build-plugin.sh` and `bash scripts/test-baseline.sh` completed during resume. Actual host mGBA is 0.10.2.
- **Recent work:** Corrected both screenshot scripts. Stella’s rendered bounds map into the plugin viewport at (35,25), 160x90, on a 240x160 canvas. Stella hash: `f729700554bcf6027da47a7b062391522024e23a75f2b89018c0c15f5817bba2`. mGBA F12 image hash: `829ba115435c124a40bcad650c6ec61ac2c8c747b813b8aa32fb525c5ff653c0`; both nonblank, game states unsynchronized.
- **Recent work:** Current DSSMM tags/build hashes and fixture results are recorded in `PROGRESS.md` and `harnesses/cards.json`. SuperFW target changes require `make clean` between `BOARD` values. The SuperCard host model now uses two writes of `0xA55A`, then two equal mode writes, matching SuperFW Build769 source; this does not model Chis NOR map or the bit-banged SD protocol.
- **Uncommitted:** Current SuperCard model/CMake/docs correction is being committed. Ignored DSSMM/mGBA clones and test outputs remain local.
- **Crash recovery and resume:**
1. `git status --short --branch && git log -5 --oneline`; read `AGENTS.md`, `HANDOFF.md`, `PROGRESS.md`.
2. Verify `command -v docker xvfb-run xdotool; /usr/games/mgba --version`.
3. Recreate devkitARM wrapper if absent:
   ```sh
   mkdir -p /tmp/a26-make-container
   cat > /tmp/a26-make-container/make <<'SH'
   #!/usr/bin/env bash
   set -euo pipefail
   exec docker run --rm -v /workspaces/a26.gba:/workspaces/a26.gba -w "$PWD" devkitpro/devkitarm:latest bash -lc 'git config --global --add safe.directory /workspaces/a26.gba/harnesses/third_party/dssmm; export PATH="$DEVKITARM/bin:$DEVKITPRO/tools/bin:$PATH"; exec make "$@"' a26-make "$@"
   SH
   chmod +x /tmp/a26-make-container/make
   ```
   Native SuperFW preview scripts also need host-accessible `arm-none-eabi-nm`, `arm-none-eabi-objcopy`, and `arm-none-eabi-gdb`. If absent, create same-shape wrappers under `/tmp/a26-container-tools/` that invoke each named tool in `devkitpro/devkitarm:latest`, mounting `/workspaces/a26.gba` and setting `PATH="$DEVKITARM/bin:$DEVKITPRO/tools/bin:$PATH"`. These are session-local.
4. If missing, clone DSSMM: `git clone https://github.com/voluptuary/dssmm.git harnesses/third_party/dssmm`. Current release tags and full commits are in `harnesses/cards.json`; fetch tags if needed.
5. For each EZ profile, checkout its exact release tag from `harnesses/cards.json`, then from DSSMM root run `PATH=/tmp/a26-make-container:$PATH MGBA_BIN=/usr/games/mgba bash <profile>/scripts/test-native-launch-mgba.sh`. Preserve/rename stale generated `build-launch` directories if `.d` files refer to `/work`. Scripts currently label mGBA 0.10.1 while this host runs 0.10.2.
6. SuperCard profiles use DSSMM SuperFW `superfw-build769`, with `BOARD=chis`, `BOARD=sd`, and `BOARD=lite`. **Run `make clean` before every BOARD switch** because Make shares profile-specific intermediate names. From `harnesses/third_party/dssmm/superfw`, build using `PATH=/tmp/a26-make-container:$PATH make BOARD=<board> DSSMM_BUILD_NUMBER=769`, then run `PATH=/tmp/a26-container-tools:$PATH MGBA_BIN=/usr/games/mgba bash scripts/test-native-settings-mgba.sh`. All three clean boards passed the two-boot native settings fixture; Chis also passed menu scenes 1,2,3,5-14 and NOR scenes 16-19. Existing fixture scope is documented in `scripts/test-native-settings-mgba.sh` and `docs/MGBA-TESTING.md`; it uses virtual SRAM SD and does not emulate the card SD bus.
7. Build the standalone model with `cmake -S harnesses -B build/harness-cmake -G Ninja && cmake --build build/harness-cmake --target supercard-model-test && build/harness-cmake/supercard-model-test`. The CMake runner is conditional because `harnesses/src/mgba_supercard_runner.c` does not exist yet. The host model's register protocol is source-qualified against DSSMM SuperFW only; real SD bus behavior is not implemented.
8. Fresh Codespace setup is in `PROGRESS.md` section 3. Plugin build: `bash scripts/build-plugin.sh`; smoke baseline: `bash scripts/test-baseline.sh`. Do not commit BIOS, card/kernel binaries, generated SD images or ignored source clones.
- **Priority:** Complete and verify GBA + six card profiles + SD images before plugin optimization. The harness must represent documented hardware; plugin fixes belong in plugin source. Stella/mGBA are desktop references only; real hardware remains final authority.
