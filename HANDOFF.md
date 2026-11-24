# Crash Recovery

- **Doing now:** Performance work for Atari 2600 on GBA, prioritizing responsive input and stable 30 fps game time with synchronized audio; frame skipping is acceptable to preserve timing.
- **Exact next step:** Fix or replace the standalone fixture capture that currently produces a black 240x160 image, then establish a comparable rendered-frame and execution-time baseline before making emulator hot-path changes.
- **Branch:** `main`
- **Last known commit:** `85f8141`
- **Build:** `bash scripts/build-plugin.sh` completed after resume; baseline command `bash scripts/test-baseline.sh` completed.
- **Recent work:** Corrected both screenshot scripts. Stella’s rendered bounds are mapped into the observed 160x90 plugin viewport at (35,25) on a 240x160 canvas. Stella image SHA-256: `f729700554bcf6027da47a7b062391522024e23a75f2b89018c0c15f5817bba2`. `capture-fixture.py` now reads mGBA’s F12 emulator PNG directly (previously it sampled the desktop top-left); mGBA image SHA-256: `829ba115435c124a40bcad650c6ec61ac2c8c747b813b8aa32fb525c5ff653c0`. Both frames are nonblank, but animation state alignment is not verified.
- **Uncommitted:** `PROGRESS.md`, `scripts/capture-stella-reference.py`, `scripts/capture-fixture.py`.
- **Resume commands:** `bash scripts/capture-stella-reference.sh`; run both capture scripts, then inspect the `do_screen()` frame boundary in `source/atari2600/vmachine.c` and establish an execution timing counter.
- **Limits:** Stella/mGBA are desktop references only; real GBA/card speed, input feel, and audio sync need hardware qualification. Do not claim measured speed gains without a reliable metric.
