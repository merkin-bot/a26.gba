# Crash Recovery

- **Doing now:** Hardware faithful mGBA harness work for EZ-FLASH Omega and SuperCard. Do not resume plugin optimizations until the harness is verified; do not alter harness behavior to compensate for plugin problems.
- **Exact next step:** Validate current published releases, not the older test snapshot: EZ DE `v8.4.1` (`a948b936`), Original OMEGA `v8.4.1-omega-test5` (`29bbb055`), BLUE `v8.4.1-blue-test5` (`9bc24e04`), and SuperFW `superfw-build769` (`37c9e069`). The prior five-case EZ launch passes were on untagged older snapshot `f6350776`; rerun after checking out each exact EZ tag. Then build SuperFW `chis`, `sd`, and `lite` from Build769 and reuse its mGBA fixtures. Those EZ fixtures stop at mocked card handoff; full SD/card I/O remains unverified. Full kernel/card/SD integration remains unimplemented. `harnesses/CMakeLists.txt` references missing runner source. Do not resume plugin tuning until harness is verified.
- **Branch:** `main`
- **Last known commit:** `76e9f4e` (pushed to origin/main)
- **Build:** `bash scripts/build-plugin.sh` and `bash scripts/test-baseline.sh` completed during resume. Actual host mGBA is 0.10.2.
- **Recent work:** Corrected both screenshot scripts. Stella’s rendered bounds map into the plugin viewport at (35,25), 160x90, on a 240x160 canvas. Stella hash: `f729700554bcf6027da47a7b062391522024e23a75f2b89018c0c15f5817bba2`. mGBA F12 image hash: `829ba115435c124a40bcad650c6ec61ac2c8c747b813b8aa32fb525c5ff653c0`; both nonblank, game states unsynchronized.
- **Uncommitted:** None after `3c71a24`. Ignored test outputs and DSSMM clone remain local.
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
4. If missing, clone DSSMM: `git clone https://github.com/voluptuary/dssmm.git harnesses/third_party/dssmm`. Current release tags and full commits are in `harnesses/cards.json`; fetch tags if needed.
5. For each EZ profile, checkout its exact release tag from `harnesses/cards.json`, then from DSSMM root run `PATH=/tmp/a26-make-container:$PATH MGBA_BIN=/usr/games/mgba bash <profile>/scripts/test-native-launch-mgba.sh`. Preserve/rename stale generated `build-launch` directories if `.d` files refer to `/work`. Scripts currently label mGBA 0.10.1 while this host runs 0.10.2.
6. SuperCard profiles use DSSMM SuperFW `superfw-build769`, with `BOARD=chis`, `BOARD=sd`, and `BOARD=lite`; existing preview/menu tests are documented at `harnesses/third_party/dssmm/superfw/docs/MGBA-TESTING.md`. Build via `PATH=/tmp/a26-make-container:$PATH make BOARD=<board> DSSMM_BUILD_NUMBER=769`; fixtures do not exercise the SD/card bus.
8. Fresh Codespace setup is in `PROGRESS.md` section 3. Plugin build: `bash scripts/build-plugin.sh`; smoke baseline: `bash scripts/test-baseline.sh`. Do not commit BIOS, card/kernel binaries, generated SD images or ignored source clones.
- **Priority:** Complete and verify GBA + six card profiles + SD images before plugin optimization. The harness must represent documented hardware; plugin fixes belong in plugin source. Stella/mGBA are desktop references only; real hardware remains final authority.
