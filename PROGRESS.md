# PROGRESS.md

Handoff document for any AI agent or human continuing this project. Read `AGENTS.md` first, then this file in full.

**Last updated:** YYYY-MM-DD HH:MM UTC by <agent/human>
**Current branch:** <branch>
**Last pushed commit:** <hash>

> Rules: update this file before starting each task and right after finishing each step. Commit and push it often. Delete stale info rather than piling up contradictions. Mark anything not confirmed as `UNVERIFIED`.

---

## 1. Project summary

- **Goal:** Atari 2600 emulator for the Game Boy Advance, running as a PogoShell plugin on EZ-FLASH Omega family and SuperFlash family devices. Continues https://github.com/voluptuary/dssmm.
- **Status:** Runs on unpublished SuperFlash build 822, but slowly, with no sound, and menu parity with other PogoShell plugins (e.g. Goomba) is incomplete (different menu font).
- **Current focus:** <one line>

---

## 2. Quick start for a new agent

1. Read `AGENTS.md`, then this file.
2. `git status` and `git log -5 --oneline`; reconcile with section 6.
3. If the environment is fresh, follow section 3.
4. Run the build and test commands in section 3.4 to confirm the baseline.
5. Resume the item under "In progress" in section 6 using its resume command.

---

## 3. Dev environment setup (GitHub Codespaces)

### 3.1 Base environment
- Devcontainer: `.devcontainer/devcontainer.json` <TODO: create/confirm>
- Setup script: `.devcontainer/setup.sh` <TODO>
- Base image: <TODO>

### 3.2 Toolchain
- devkitPro / devkitARM version: <TODO>
- Install method: <TODO exact commands>
- Environment variables: `DEVKITPRO=<path>`, `DEVKITARM=<path>`, PATH additions: <TODO>
- apt packages: <TODO: build-essential, cmake, ninja, git, SDL2/Qt or headless deps, xvfb, etc.>

### 3.3 Repo contents and third-party assets
- Unpack `a26.zip`: `<TODO exact command>` -> contents live in `<path>`
- PogoShell and other required third-party binaries: where to get them, where to place them, license notes. **Do not commit** anything whose redistribution is not permitted.
- Open-source test game: `<path>`, license: <TODO>

### 3.4 Build and test commands
- Build plugin: `<TODO>`
- Build patched mGBA (with SuperCard/EZ-Flash harnesses): `<TODO>`
- Run harness headlessly (xvfb or no-GUI build, screenshot/log capture): `<TODO>`
- Run tests: `<TODO>`

### 3.5 Codespaces limitations hit
- No USB passthrough to real hardware, storage limits, idle timeout, etc.: <TODO, add as discovered, with workarounds>

---

## 4. Repository map

| Path | Purpose |
|------|---------|
| `a26.zip` | Prior source, open-source test game, text file with relevant repo links |
| `AGENTS.md` | Standing instructions for agents |
| `PROGRESS.md` | This file |
| <TODO> | <TODO> |

### Referenced repositories (from the link text file in `a26.zip`)

| Repo | What it is | License | Used for |
|------|-----------|---------|----------|
| <TODO> | | | |

---

## 5. Hardware/software knowledge base

Each fact needs a source or evidence. Separate verified from unverified.

### VERIFIED
- <fact> (source: <file/repo/test/hardware observation>)

### UNVERIFIED
- <assumption> (how to verify: <plan>)

Topics to cover: PogoShell plugin ABI (load, launch, ROM handoff, save/state memory, return to shell); EZ-Flash Omega and SuperCard/SuperFlash behavior (SD/FAT access, save types, waitstates); memory layout used by the plugin; timing budgets (cycles per scanline/frame); TIA audio plan (Direct Sound/PSG).

---

## 6. Task board

### In progress
- **Task:** <name>
- **Exact step:** <what you are doing right now>
- **Files touched:** <list>
- **Uncommitted changes:** <yes/no, summary>
- **Branch:** <branch>
- **Resume command:** `<command>`

### Next up (ordered)
1. Onboard and audit: unpack `a26.zip`, map repo, reproducible Codespace build.
2. Test harnesses: finish/verify mGBA SuperCard and EZ-Flash harnesses; headless run with screenshots/frame hashes.
3. Baseline measurements (performance + screenshot/frame-hash), before any emulation changes.
4. Performance: reach full speed (60 fps NTSC or documented frame-skip).
5. Sound: TIA audio via Direct Sound and/or PSG.
6. Menu/UI parity with Goomba (font first, then layout, navigation, options, save-state conventions).
7. Compatibility and polish: bankswitching, PAL/NTSC, controllers, per-game settings, saves.

### Backlog
- <ideas not yet scheduled>

### Done
| Date | Task | Commit | Notes |
|------|------|--------|-------|
| | | | |

---

## 7. Performance log

| Date | Commit | Test ROM | Metric | Method | Result |
|------|--------|----------|--------|--------|--------|
| | | | | | |

---

## 8. Test status

| Test/harness | How to run | Status | Notes |
|--------------|-----------|--------|-------|
| | | | |

### Known harness-vs-hardware differences
- <difference, impact, what the plugin must not be tuned to>

---

## 9. Known issues and decisions

### Issues
- <bug, repro steps, suspected cause>

### Dead ends (do not repeat)
- <what was tried, why it failed>

### Design decisions
- <decision> because <reason>

### Licensing notes
- <repo/code reuse constraints>

---

## 10. Open questions for the owner

1. <question> (blocked tasks: <list>; working on <other task> meanwhile)

---

## 11. Session log

Short dated entries. Trim old entries periodically.

- YYYY-MM-DD: <what was done>. Next: <what is next>.
