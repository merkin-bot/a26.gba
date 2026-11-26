# Session kickoff

On a cold start, saying “Read AGENTS.md” is sufficient. Follow the instructions there without waiting for another prompt.

1. Read `AGENTS.md`, this file, `PROGRESS.md`, then `WORKSPACE.md` completely.
2. Run `git status --short --branch` and `git log -5 --oneline`; reconcile repository state with `PROGRESS.md`.
3. Confirm the host matches `WORKSPACE.md`. If tools or pinned clones are missing, follow its host setup and run the baseline. Do not use `.devcontainer` or Docker-in-Docker.
4. Resume the exact **Current task / Next action** in `PROGRESS.md`. Update that file before and after each step. Continue autonomously; do not ask the owner to restate context.

## Current checkpoint

- Current task and next action: `PROGRESS.md` §1. StellaDS architecture audit is complete. Local-PC overflowed IWRAM; inline and shared-helper RIOT store experiments are rejected (inline is unsafe; helper was slower). Owner requested GPU offload exploration. A packed Mode 4 indexed-output prototype was slower (~7% in one mGBA window) and mismatched framebuffer output, so it was reverted. GBA OBJ access would require H-Blank processing changes and a new mapping from asynchronous Atari scanlines to LCD scanlines, while collision logic remains CPU-side. No direct OBJ offload was identified. Next resume guest CPU/TIA profiling; a line-buffer redesign would need its own timing and pixel-fidelity gate.
- Build environment and exact commands: `WORKSPACE.md`.
- Latest generated plugin copy: `candidate/a26.gba`; it is versioned and `scripts/build-plugin.sh` refreshes it after each production build.
- Current candidate SHA-256: `4baab3c9a0445d5ff6be1cbe0607789def30c8572f8d449620e37c7aa522d8e5`; both `scripts/test-baseline.sh` and the fresh six-profile shared-image mGBA run passed on 2026-11-26. EZ handoff remains UNVERIFIED.
- Card profiles and harness evidence: `harnesses/cards.json`, `harnesses/README.md`, `docs/provenance/`.
- Latest commit and verified/unverified results: run `git log`; see `PROGRESS.md`.
- Offline workspace cleanup and baseline passed in `114f259`. The O3 default, Direct Sound register change, and current audio-table plugin pass `scripts/test-baseline.sh`; current SHA `4baab3c9a0445d5ff6be1cbe0607789def30c8572f8d449620e37c7aa522d8e5` also passed the six-profile shared exFAT mGBA harness. EZ handoff remains unverified. `mainloop()`, `do_screen()`, and `decRead()` run from IWRAM. O3 averaged 2,989 vs. 3,025 mGBA frames at the same Atari boundary as O2, with identical output. mGBA now captures nonzero PCM after the Direct Sound setup fix, but the 50-second capture has a long silent prefix while Atari advances only about 0.67 seconds; synchronized or continuous sound is not achieved. Current task: profile larger interpreter/TIA costs to address the remaining roughly tenfold mGBA-cycle gap, then redesign audio scheduling against emulated Atari time, per `PROGRESS.md` §1 and `docs/provenance/tia-audio.md`. Physical GBA timing and audible hardware sound remain `UNVERIFIED`. Full offline commands are in `WORKSPACE.md`.

This file is intentionally a short recovery index. Project requirements belong in `AGENTS.md`; detailed current state and evidence belong in `PROGRESS.md`; environment setup belongs in `WORKSPACE.md`.
