# Session kickoff

On a cold start, saying “Read AGENTS.md” is sufficient. Follow the instructions there without waiting for another prompt.

1. Read `AGENTS.md`, this file, `PROGRESS.md`, then `WORKSPACE.md` completely.
2. Run `git status --short --branch` and `git log -5 --oneline`; reconcile repository state with `PROGRESS.md`.
3. Confirm the host matches `WORKSPACE.md`. If tools or pinned clones are missing, follow its host setup and run the baseline. Do not use `.devcontainer` or Docker-in-Docker.
4. Resume the exact **Current task / Next action** in `PROGRESS.md`. Update that file before and after each step. Continue autonomously; do not ask the owner to restate context.

## Current checkpoint

- Current task and next action: `PROGRESS.md` §1.
- Build environment and exact commands: `WORKSPACE.md`.
- Card profiles and harness evidence: `harnesses/cards.json`, `harnesses/README.md`, `docs/provenance/`.
- Latest commit and verified/unverified results: run `git log`; see `PROGRESS.md`.
- Offline workspace cleanup and baseline passed in `114f259`. The shared-image harness gate passes in mGBA; EZ plugin handoff on the common image remains unverified. The Atari baseline and placement iterations are in `docs/provenance/atari-performance-baseline.md`. `mainloop()`, `do_screen()`, and `decRead()` currently run from IWRAM; three decRead replays averaged 3,025 mGBA frames to Atari TV counter 41 vs. 3,036.7 for the prior candidate, with identical captures and about 908 bytes above the lowest sampled SP. Current task: inspect dispatch/code-mode and larger per-opcode costs for a meaningful measured improvement, per `PROGRESS.md` §1. Physical GBA timing and physical-card behavior remain `UNVERIFIED`. Full offline commands are in `WORKSPACE.md`.

This file is intentionally a short recovery index. Project requirements belong in `AGENTS.md`; detailed current state and evidence belong in `PROGRESS.md`; environment setup belongs in `WORKSPACE.md`.
