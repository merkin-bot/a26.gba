# Crash Recovery

- **Doing now:** Initial audit: archive extraction, repository inventory, and reproducible build setup.
- **Exact next command:** `git --no-pager status --short --branch && bash .devcontainer/setup.sh && bash archive-inputs/test-source/atari2600/build.sh`
- **Branch:** `main`
- **Last pushed commit:** `f9c87d7`
- **Build passes:** Verified with Docker devkitARM 16.1.0; output SHA-256 matches archive plugin.
- **Half-finished:**
  - Read AGENTS.md and template PROGRESS.md.
  - Extracted `a26.zip` under `archive-inputs/`; read `repositories.txt`.
  - No HANDOFF existed; it is being initialized.
  - Prepared source, test ROM, plugin binary, GPLv3 text, and notices are under `archive-inputs/`.
- **Known blocker:** PogoShell/mGBA harness not found or built; stock mGBA only can validate the plugin binary, not firmware ABI.
- **Resume:** Finish devcontainer setup, build `archive-inputs/test-source/atari2600/prototype.gba`, and reconcile baseline evidence.
