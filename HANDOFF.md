# Crash Recovery

- **Doing now:** Flashcard/harness audit: FAT32, portable ABI, and cold-boot fixture pass; full card I/O remains.
- **Exact next command:** `bash scripts/run-card-fixture.sh 5`
- **Branch:** `main`
- **Last pushed commit:** `cf5e58f`
- **Build passes:** Normal plugin and standalone fixture both build; fixture survives 3 seconds in mGBA 0.10.2/Xvfb.
- **Half-finished:**
  - Read AGENTS.md and template PROGRESS.md.
  - Extracted `a26.zip` under `archive-inputs/`; read `repositories.txt`.
  - No HANDOFF existed; it is being initialized.
  - Prepared source, test ROM, plugin binary, GPLv3 text, and notices are under `archive-inputs/`.
- **Known blocker:** Full card launch still needs mGBA SuperCard/Chis/NOR/SD protocol hooks; DSSMM documents that stock mGBA cannot emulate card I/O.
- **Resume:** Run baseline, then implement PogoShell/mGBA cartridge and shell launch fixture; do not infer hardware behavior from stock mGBA.
