# Session kickoff

On a cold start, saying “Read AGENTS.md” is sufficient. Follow the instructions there without waiting for another prompt.

1. Read `AGENTS.md`, this file, `PROGRESS.md`, then `WORKSPACE.md` completely.
2. Run `git status --short --branch` and `git log -5 --oneline`; reconcile repository state with `PROGRESS.md`.
3. Confirm the host matches `WORKSPACE.md`. If tools or pinned clones are missing, follow its host setup and run the baseline. Do not use `.devcontainer` or Docker-in-Docker.
4. Resume the exact **Current task / Next action** in `PROGRESS.md`. Update that file before and after each step. Continue autonomously; do not ask the owner to restate context.

## Current checkpoint

- Current task and next action: `PROGRESS.md` §1. Production Mode 3 candidate SHA-256 is `2b22e6ad4d1391efb4b9071c8c5f6c59c0de9eef052fc843f5480d155ca0d258`; `scripts/test-baseline.sh` and all six shared-image profile lanes pass. The exact per-line `unified_count` snapshot measured 116,551,386 vs. 116,380,800 mGBA cycles (0.146% fewer), with matched Atari and pixel-cache state. The latest completed PPU experiment, pair-packed Mode 4, measured 113,108,737 cycles (2.81% fewer than Mode 3) but differed in 32 scanout pixels, so it is not accepted for production. The mode4 switch remains opt-in for diagnosis. Next investigate whether a scanline-synchronous GBA BG path can offload TIA priority/color composition while preserving collisions and mid-line register changes. Physical GBA timing remains UNVERIFIED.
- Build environment and exact commands: `WORKSPACE.md`.
- Latest generated plugin copy: `candidate/a26.gba`; it is versioned and `scripts/build-plugin.sh` refreshes it after each production build.
- Current candidate SHA-256: `2b22e6ad4d1391efb4b9071c8c5f6c59c0de9eef052fc843f5480d155ca0d258`; both `scripts/test-baseline.sh` and the fresh six-profile shared-image mGBA run passed on 2026-11-26. EZ handoff remains UNVERIFIED.
- Card profiles and harness evidence: `harnesses/cards.json`, `harnesses/README.md`, `docs/provenance/`.
- Latest commit and verified/unverified results: run `git log`; see `PROGRESS.md`.
- Offline workspace cleanup and baseline passed in `114f259`. The current task and audio state are tracked in `PROGRESS.md` §1. CPU-side measured work has reduced the matched Atari window from 153,937,508 cycles to 116,380,800 cycles through IWRAM placement and the per-line count snapshot; this is mGBA model evidence only. The broad original baseline was about 6.21 million modeled cycles per Atari frame (2.71 fps on mGBA's GBA-frame timeline); the present exact window implies roughly 3.75 million per Atari frame, an indicative ~39.5% reduction / ~1.65x throughput. This is a cross-window estimate, not a directly matched whole-frame result. A 30 fps target is still far away. Audio remains unsynchronized and physical timing/sound remain UNVERIFIED. Full offline commands are in `WORKSPACE.md`.

This file is intentionally a short recovery index. Project requirements belong in `AGENTS.md`; detailed current state and evidence belong in `PROGRESS.md`; environment setup belongs in `WORKSPACE.md`.
