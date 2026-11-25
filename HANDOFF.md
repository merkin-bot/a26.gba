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
- Offline workspace cleanup and baseline passed in `114f259`. The shared-image harness gate now passes in mGBA for all six profiles using one 4 GiB exFAT image with 32 KiB clusters; run `scripts/verify-six-profile-image.sh`. The EZ shared-image scenario ends at a kernel save-error dialog and does not prove EZ plugin handoff. The repeatable three-run Atari baseline is recorded in `docs/provenance/atari-performance-baseline.md`. Current task: profile the linked ARM interpreter/render paths and select a measured optimization, per `PROGRESS.md` §1. Keep physical GBA timing and physical-card behavior marked `UNVERIFIED`; update the handoff after each completed task. Full offline commands are in `WORKSPACE.md`.

This file is intentionally a short recovery index. Project requirements belong in `AGENTS.md`; detailed current state and evidence belong in `PROGRESS.md`; environment setup belongs in `WORKSPACE.md`.
