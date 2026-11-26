# Session kickoff

On a cold start, saying “Read AGENTS.md” is sufficient. Follow the instructions there without waiting for another prompt.

1. Read `AGENTS.md`, this file, `PROGRESS.md`, then `WORKSPACE.md` completely.
2. Run `git status --short --branch` and `git log -5 --oneline`; reconcile repository state with `PROGRESS.md`.
3. Confirm the host matches `WORKSPACE.md`. If tools or pinned clones are missing, follow its host setup and run the baseline. Do not use `.devcontainer` or Docker-in-Docker.
4. Resume the exact **Current task / Next action** in `PROGRESS.md`. Update that file before and after each step. Continue autonomously; do not ask the owner to restate context.

## Current checkpoint

- Current task and next action: `PROGRESS.md` §1. Production Mode 3 candidate SHA-256 is `5e33206de61b1534c350fe9c6221f2d5180627ae1da8a1adf327f489c6ff97f8`; `scripts/test-baseline.sh` and all six shared-image profile lanes pass. The exact per-line `unified_count` snapshot measured 116,551,386 vs. 116,380,800 mGBA cycles (0.146% fewer), with matched Atari and pixel-cache state. The latest completed PPU experiment, pair-packed Mode 4, was 2.81% lower in its matched mGBA window but differed in 32 scanout pixels; a separate 64 MiB run further exposed a scanout sampling mismatch. Neither result is a production candidate. LTO was rejected after start-state matches but output/game-state divergence; details are in the performance provenance. The event-aware `CTRLPF` local measured 114,499,940 vs. 115,357,494 O3 mGBA cycles (0.743% fewer) in three state/output-matched runs; sampled min SP `0x03007DC0`, static end `0x03007D54` (108-byte observed gap, not worst-case). Next profile another bounded CPU/TIA hot path using exact state and output checks. Physical GBA timing remains UNVERIFIED.
- Build environment and exact commands: `WORKSPACE.md`.
- Latest generated plugin copy: `candidate/a26.gba`; it is versioned and `scripts/build-plugin.sh` refreshes it after each production build.
- Current candidate SHA-256: `5e33206de61b1534c350fe9c6221f2d5180627ae1da8a1adf327f489c6ff97f8`; both `scripts/test-baseline.sh` and the fresh six-profile shared-image mGBA run passed on 2026-11-26. EZ handoff remains UNVERIFIED.
- Card profiles and harness evidence: `harnesses/cards.json`, `harnesses/README.md`, `docs/provenance/`.
- Latest commit and verified/unverified results: run `git log`; see `PROGRESS.md`.
- Offline workspace cleanup and baseline passed in `114f259`. The current task and audio state are tracked in `PROGRESS.md` §1. The accepted event-aware candidate measured 31 Atari frames per 407 mGBA GBA frames (about 4.55 Atari fps, roughly 68% above the historical 2.71 fps baseline); this is diagnostic mGBA evidence, not physical timing. LTO was rejected because output/state diverged. A 30 fps target is still far away. Audio remains unsynchronized and physical timing/sound remain UNVERIFIED. Full offline commands are in `WORKSPACE.md`.

This file is intentionally a short recovery index. Project requirements belong in `AGENTS.md`; detailed current state and evidence belong in `PROGRESS.md`; environment setup belongs in `WORKSPACE.md`.
