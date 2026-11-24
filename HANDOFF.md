# Crash Recovery

- **Doing now:** Porting/harness: FAT32, PogoShell ABI, cold-boot, and host SuperCard mode/SDRAM/SD model pass.
- **Exact next command:** `bash scripts/run-card-fixture.sh 5`
- **Branch:** `main`
- **Last pushed commit:** `ef46bb9`
- **Build passes:** Normal plugin and standalone fixture both build; fixture survives 3 seconds in mGBA 0.10.2/Xvfb.
- **Half-finished:**
  - Read AGENTS.md and template PROGRESS.md.
  - Ingested the original archive into canonical directories.
  - Prepared source, test ROM, plugin binary, GPLv3 text, and notices are organized under `source/`, `dist/`, `test-roms/`, and `docs/provenance/`.
- **Known blocker:** Full card launch still needs mGBA memory hooks and Omega/SuperFlash firmware register evidence.
- **Resume:** Run baseline, then implement PogoShell/mGBA cartridge and shell launch fixture; do not infer hardware behavior from stock mGBA.
