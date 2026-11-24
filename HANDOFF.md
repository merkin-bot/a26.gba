# Crash Recovery

- **Doing now:** Hardware faithful mGBA harness work for EZ-FLASH Omega and SuperCard. Do not resume plugin optimizations until the harness is verified; do not alter harness behavior to compensate for plugin problems.
- **Exact next step:** Reuse existing DSSMM mGBA kernel launch tests, finish recording EZ-DE/Original/BLUE run results, then identify uncovered GBA/card/SD paths. Six exact kernel profiles are in `harnesses/cards.json`. Full kernel/card/SD integration remains unimplemented. `harnesses/CMakeLists.txt` references missing runner source. Do not resume plugin tuning until harness is verified.
- **Branch:** `main`
- **Last known commit:** `85f8141`
- **Build:** `bash scripts/build-plugin.sh` completed after resume; baseline command `bash scripts/test-baseline.sh` completed.
- **Recent work:** Corrected both screenshot scripts. Stella’s rendered bounds map into the plugin viewport at (35,25), 160x90, on a 240x160 canvas. Stella hash: `f729700554bcf6027da47a7b062391522024e23a75f2b89018c0c15f5817bba2`. mGBA F12 image hash: `829ba115435c124a40bcad650c6ec61ac2c8c747b813b8aa32fb525c5ff653c0`; both nonblank, game states unsynchronized.
- **Uncommitted:** `PROGRESS.md`, `scripts/capture-stella-reference.py`, `scripts/capture-fixture.py`.
- **Resume commands:** `bash scripts/capture-stella-reference.sh`; run both capture scripts, then inspect the `do_screen()` frame boundary in `source/atari2600/vmachine.c` and establish an execution timing counter.
- **Priority:** Complete and verify GBA + six card profiles + SD images before plugin optimization. The harness must represent documented hardware; plugin fixes belong in plugin source. Stella/mGBA are desktop references only; real hardware remains final authority.
