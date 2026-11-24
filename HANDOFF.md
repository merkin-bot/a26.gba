# Crash Recovery

- **Doing now:** Hardware faithful mGBA harness work for EZ-FLASH Omega and SuperCard. Do not resume plugin optimizations until the harness is verified; do not alter harness behavior to compensate for plugin problems.
- **Exact next step:** Finish the BLUE DSSMM native launch suite, then map the still-missing full GBA/card/SD path. DE and Original OMEGA each passed five launch cases; traces reach mocked cartridge handoff only. Six kernel profiles are in `harnesses/cards.json`. Full kernel/card/SD integration remains unimplemented. `harnesses/CMakeLists.txt` references missing runner source. Do not resume plugin tuning until harness is verified.
- **Branch:** `main`
- **Last known commit:** `3c71a24` (pushed to origin/main)
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
   exec docker run --rm -v /workspaces/a26.gba:/workspaces/a26.gba -w "$PWD" devkitpro/devkitarm:latest make "$@"
   SH
   chmod +x /tmp/a26-make-container/make
   ```
4. If missing, clone DSSMM and pin the snapshot: `git clone https://github.com/voluptuary/dssmm.git harnesses/third_party/dssmm && git -C harnesses/third_party/dssmm checkout f6350776f07eba91ba2ffa34c6833d85f348f2a1`.
5. From `harnesses/third_party/dssmm`, run `PATH=/tmp/a26-make-container:$PATH MGBA_BIN=/usr/games/mgba bash ezfblue/scripts/test-native-launch-mgba.sh`. For stale `/work` `.d` paths, preserve/rename `ezfblue/targets/blue/build-launch` before retry. Inspect JSON/logs; note the script labels mGBA 0.10.1 while this host runs 0.10.2.
6. Fresh Codespace setup is in `PROGRESS.md` section 3. Plugin build: `bash scripts/build-plugin.sh`; smoke baseline: `bash scripts/test-baseline.sh`. Do not commit BIOS, card/kernel binaries, generated SD images or ignored source clones.
- **Priority:** Complete and verify GBA + six card profiles + SD images before plugin optimization. The harness must represent documented hardware; plugin fixes belong in plugin source. Stella/mGBA are desktop references only; real hardware remains final authority.
