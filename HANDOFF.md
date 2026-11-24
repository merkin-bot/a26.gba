# Crash Recovery

- **Doing now:** Porting/harness: baseline placement measured and IWRAM candidate rejected pending real speed evidence.
- **Exact next command:** `bash scripts/run-card-fixture.sh 5`
- **Branch:** `main`
- **Last pushed commit:** `4fdc93d`
- **Build passes:** Normal plugin and standalone fixture both build; fixture survives 3 seconds in mGBA 0.10.2/Xvfb.
- **Half-finished:**
  - Read AGENTS.md and template PROGRESS.md.
  - Ingested the original archive into canonical directories.
  - Prepared source, test ROM, plugin binary, GPLv3 text, and notices are organized under `source/`, `dist/`, `test-roms/`, and `docs/provenance/`.
- **Known blocker:** Full card launch still needs mGBA memory hooks and Omega/SuperFlash firmware register evidence.
- **Resume:** Run baseline, then implement PogoShell/mGBA cartridge and shell launch fixture; do not infer hardware behavior from stock mGBA.
