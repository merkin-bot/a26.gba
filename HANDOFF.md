# Session kickoff

On a cold start, saying “Read AGENTS.md” is sufficient. Follow the instructions there without waiting for another prompt.

1. Read `AGENTS.md`, this file, `PROGRESS.md`, then `WORKSPACE.md` completely.
2. Run `git status --short --branch` and `git log -5 --oneline`; reconcile repository state with `PROGRESS.md`.
3. Confirm the host matches `WORKSPACE.md`. If tools or pinned clones are missing, follow its host setup and run the baseline. Do not use `.devcontainer` or Docker-in-Docker.
4. Resume the exact **Current task / Next action** in `PROGRESS.md`. Update that file before and after each step. Continue autonomously; do not ask the owner to restate context.

## Current checkpoint

- Current task and next action: `PROGRESS.md` §1. The token-limit raster fast-path candidate was rebuilt and measured on 2026-11-26; it had no measured gain and has been reverted.
- Build environment and exact commands: `WORKSPACE.md`.
- Latest generated plugin copy: `candidate/a26.gba`; `scripts/build-plugin.sh` refreshes it after each production build. The binary is ignored by Git.
- Card profiles and harness evidence: `harnesses/cards.json`, `harnesses/README.md`, `docs/provenance/`.
- Latest commit and verified/unverified results: run `git log`; see `PROGRESS.md`.
- Offline workspace cleanup and baseline passed in `114f259`. The O3 default and current Direct Sound register change pass `scripts/test-baseline.sh`; the audio-change plugin also passes the six-profile shared exFAT mGBA harness. EZ handoff remains unverified. `mainloop()`, `do_screen()`, and `decRead()` run from IWRAM. O3 averaged 2,989 vs. 3,025 mGBA frames at the same Atari boundary as O2, with identical output. mGBA now captures nonzero PCM after the Direct Sound setup fix, but the 50-second capture has a long silent prefix while Atari advances only about 0.67 seconds; synchronized or continuous sound is not achieved. Current task: profile larger interpreter/TIA costs to address the remaining roughly tenfold mGBA-cycle gap, then redesign audio scheduling against emulated Atari time, per `PROGRESS.md` §1 and `docs/provenance/tia-audio.md`. Physical GBA timing and audible hardware sound remain `UNVERIFIED`. Full offline commands are in `WORKSPACE.md`.

This file is intentionally a short recovery index. Project requirements belong in `AGENTS.md`; detailed current state and evidence belong in `PROGRESS.md`; environment setup belongs in `WORKSPACE.md`.
