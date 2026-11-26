# Session kickoff

On a cold start, saying “Read AGENTS.md” is sufficient. Follow the instructions there without waiting for another prompt.

1. Read `AGENTS.md`, this file, `PROGRESS.md`, then `WORKSPACE.md` completely.
2. Run `git status --short --branch` and `git log -5 --oneline`; reconcile repository state with `PROGRESS.md`.
3. Confirm the host matches `WORKSPACE.md`. If tools or pinned clones are missing, follow its host setup and run the baseline. Do not use `.devcontainer` or Docker-in-Docker.
4. Resume the exact **Current task / Next action** in `PROGRESS.md`. Update that file before and after each step. Continue autonomously; do not ask the owner to restate context.

## Current checkpoint

- Current task and next action: `PROGRESS.md` §1. The safe per-line `unified_count` snapshot is now in `draw_vector1()`. A single exact PC/clock aligned run reduced mGBA cycles 0.147% (116,551,182 to 116,380,048) with matching CPU, RAM, TIA/RIOT, pixel-cache, and VRAM hashes at both boundaries. Fresh-image repeats sometimes miss the Atari start sample; those are invalid timing runs, so repeatability remains the next action. Current provisional candidate SHA-256 is `2b22e6ad4d1391efb4b9071c8c5f6c59c0de9eef052fc843f5480d155ca0d258`; run the documented baseline and six-profile checks before treating it as accepted. GPU audit: direct Mode 4 and OBJ/DMA are rejected; a future PPU priority compositor needs queued TIA changes synchronized with LCD lines and collision/image parity. StellaDS local-PC overflowed IWRAM; inline RIOT store was unsafe; shared helper was slower. Prior IWRAM placement `draw_vector1()` measured 24.29% fewer mGBA cycles in three matched runs per build. The `CTRLPF` hoist was rejected due cache/VRAM mismatch.
- Build environment and exact commands: `WORKSPACE.md`.
- Latest generated plugin copy: `candidate/a26.gba`; it is versioned and `scripts/build-plugin.sh` refreshes it after each production build.
- Current candidate SHA-256: `b8c08c211b69bdb8db6fdd6a8aa68c0705deb3088d566b23bc3344e07f7a7b72`; both `scripts/test-baseline.sh` and the fresh six-profile shared-image mGBA run passed on 2026-11-26. EZ handoff remains UNVERIFIED.
- Card profiles and harness evidence: `harnesses/cards.json`, `harnesses/README.md`, `docs/provenance/`.
- Latest commit and verified/unverified results: run `git log`; see `PROGRESS.md`.
- Offline workspace cleanup and baseline passed in `114f259`. The O3 default, Direct Sound register change, and current audio-table plugin pass `scripts/test-baseline.sh`; current candidate SHA `b8c08c211b69bdb8db6fdd6a8aa68c0705deb3088d566b23bc3344e07f7a7b72` also passed the six-profile shared exFAT mGBA harness. EZ handoff remains unverified. `mainloop()`, `do_screen()`, and `decRead()` run from IWRAM. O3 averaged 2,989 vs. 3,025 mGBA frames at the same Atari boundary as O2, with identical output. mGBA now captures nonzero PCM after the Direct Sound setup fix, but the 50-second capture has a long silent prefix while Atari advances only about 0.67 seconds; synchronized or continuous sound is not achieved. Current task: profile larger interpreter/TIA costs to address the remaining roughly tenfold mGBA-cycle gap, then redesign audio scheduling against emulated Atari time, per `PROGRESS.md` §1 and `docs/provenance/tia-audio.md`. Physical GBA timing and audible hardware sound remain `UNVERIFIED`. Full offline commands are in `WORKSPACE.md`.

This file is intentionally a short recovery index. Project requirements belong in `AGENTS.md`; detailed current state and evidence belong in `PROGRESS.md`; environment setup belongs in `WORKSPACE.md`.
