# Crash Recovery

- **Doing now:** Performance work for Atari 2600 on GBA, prioritizing responsive input and stable 30 fps game time with synchronized audio; frame skipping is acceptable to preserve timing.
- **Exact next step:** Fix or replace the standalone fixture capture that currently produces a black 240x160 image, then establish a comparable rendered-frame and execution-time baseline before making emulator hot-path changes.
- **Branch:** `main`
- **Last known commit:** `85f8141`
- **Build:** `bash scripts/build-plugin.sh` completed after resume; baseline command `bash scripts/test-baseline.sh` completed.
- **Recent work:** Corrected `scripts/capture-stella-reference.py` to crop Stella's rendered non-black bounds and scale to 240x160. Stella image SHA-256: `6c5acac517cdb5513b30af3cd6d89fd478d6cc178dbef79ed370d2d875ee5be2` (generated under `work/reference/`, ignored). mGBA fixture capture remains black with SHA-256 `47fbde60709a27683035d7b385f06dd1447f68c6548272758e15a335cf3bc15e`; not comparable yet.
- **Uncommitted:** `PROGRESS.md`, `scripts/capture-stella-reference.py`.
- **Resume commands:** `bash scripts/capture-stella-reference.sh`; inspect `source/atari2600/fixture.S`, `scripts/build-fixture.sh`, then `bash scripts/capture-fixture.sh`.
- **Limits:** Stella/mGBA are desktop references only; real GBA/card speed, input feel, and audio sync need hardware qualification. Do not claim measured speed gains without a reliable metric.
