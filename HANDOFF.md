# Crash Recovery

- **Doing now:** Baseline/harness audit: plugin hash verified and standalone Flappy fixture running in stock mGBA.
- **Exact next command:** `bash scripts/test-baseline.sh`
- **Branch:** `main`
- **Last pushed commit:** `cf5e58f`
- **Build passes:** Normal plugin and standalone fixture both build; fixture survives 3 seconds in mGBA 0.10.2/Xvfb.
- **Half-finished:**
  - Read AGENTS.md and template PROGRESS.md.
  - Extracted `a26.zip` under `archive-inputs/`; read `repositories.txt`.
  - No HANDOFF existed; it is being initialized.
  - Prepared source, test ROM, plugin binary, GPLv3 text, and notices are under `archive-inputs/`.
- **Known blocker:** PogoShell/mGBA firmware harness not found or built.
- **Resume:** Run baseline, then implement PogoShell/mGBA cartridge and shell launch fixture; do not infer hardware behavior from stock mGBA.
