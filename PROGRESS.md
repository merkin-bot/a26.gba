# PROGRESS.md

Handoff document for any AI agent or human continuing this project. Read `AGENTS.md` first, then this file in full.

**Last updated:** 2026-11-24 16:10 UTC by Copilot
**Current branch:** main
**Last pushed commit:** cf5e58f

> Rules: update this file before starting each task and right after finishing each step. Commit and push it often. Delete stale info rather than piling up contradictions. Mark anything not confirmed as `UNVERIFIED`.

---

## 1. Project summary

- **Goal:** Atari 2600 emulator for the Game Boy Advance, running as a PogoShell plugin on EZ-FLASH Omega family and SuperFlash family devices. Continues https://github.com/voluptuary/dssmm.
- **Status:** Runs on unpublished SuperFlash build 822, but slowly, with no sound, and menu parity with other PogoShell plugins (e.g. Goomba) is incomplete (different menu font). Existing source archive has a prepared GPLv3 source build, an experimental plugin binary, an open-source generated diagnostic, and runtime checks from stock mGBA 0.10.1.
- **Current focus:** Initial audit, reproducible Codespaces setup, source/build baseline, and repository research.

---

## 2. Quick start for a new agent

1. Read `AGENTS.md`, `HANDOFF.md`, then this file.
2. `git status` and `git log -5 --oneline`; reconcile with section 6.
3. If the environment is fresh, follow section 3.
4. Run the build and test commands in section 3.4 to confirm the baseline.
5. Resume the item under "In progress" in section 6 using its resume command.

---

## 3. Dev environment setup (GitHub Codespaces)

### 3.1 Base environment
- Devcontainer: `.devcontainer/devcontainer.json`
- Setup script: `.devcontainer/setup.sh`
- Base image: `mcr.microsoft.com/devcontainers/base:ubuntu-24.04`

### 3.2 Toolchain
- devkitPro / devkitARM version: Docker `devkitpro/devkitarm:latest`, compiler `arm-none-eabi-gcc (devkitARM) 16.1.0`; exact image digest not pinned
- Install method: `.devcontainer/setup.sh` calls the devkitPro pacman installer, then `dkp-pacman -Sy gba-dev libgba`
- Environment variables: `DEVKITPRO=/opt/devkitpro`, `DEVKITARM=/opt/devkitpro/devkitARM`, PATH additions: `$DEVKITARM/bin:$DEVKITPRO/tools/bin`
- apt packages: build-essential, cmake, ninja-build, SDL2/Qt/OpenGL, PNG/ZIP/ZLIB, FFmpeg, ELF/edit, plus devkitPro `gba-dev`

### 3.3 Repo contents and third-party assets
- Unpack `a26.zip`: `unzip -oq a26.zip -d archive-inputs` -> extracted assets live in `archive-inputs/`
- Extract test source: `unzip -oq archive-inputs/atari2600-test-source.zip -d archive-inputs/test-source`
- PogoShell and other required third-party binaries: where to get them, where to place them, license notes. **Do not commit** anything whose redistribution is not permitted.
- Open-source test game: `archive-inputs/flappy.a26`, source/license: archive-provenance `JetSetIlly/Atari2600-Flappy-Bird` BSD-2-Clause source at `/tmp/flappy` during audit; verify source matches binary by its upstream build if copied.

### 3.4 Build and test commands
- Build plugin: `<TODO>`
- Build plugin: `bash scripts/build-plugin.sh` (official `devkitpro/devkitarm:latest` Docker image)
- Build standalone stock-mGBA fixture: `bash scripts/build-fixture.sh`
- Build patched mGBA (with SuperCard/EZ-Flash harnesses): `<TODO>`
- Run stock mGBA headlessly: `bash scripts/run-mgba.sh <gba-rom> <seconds>`
- Run baseline: `bash scripts/test-baseline.sh`

### 3.5 Codespaces limitations hit
- No USB passthrough to real hardware, storage limits, idle timeout, etc.: <TODO, add as discovered, with workarounds>
- apt devkitPro installer URL was blocked with HTTP 403/Cloudflare; official Docker image `devkitpro/devkitarm:latest` is the verified fallback.

---

## 4. Repository map

| Path | Purpose |
|------|---------|
| `a26.zip` | Prior source, open-source test game, text file with relevant repo links |
| `AGENTS.md` | Standing instructions for agents |
| `PROGRESS.md` | This file |
| `HANDOFF.md` | Short crash-recovery snapshot |
| `archive-inputs/` | Extracted assets from `a26.zip` |
| `archive-inputs/test-source/atari2600/` | Prepared GPLv3 source build for the experimental plugin |
| `.devcontainer/` | Reproducible Codespaces environment setup |

### Referenced repositories (from the link text file in `a26.zip`)

| Repo | What it is | License | Used for |
|------|-----------|---------|----------|
| [MikeDX/2600_gba](https://github.com/MikeDX/2600_gba) | Starting upstream; legacy GBA emulator source | GPLv2 according to GitHub metadata but source archive notes GNU GPL without numbered version | Starting point, CPU/core/TIA/legacy build |
| [Sterophonick/PocketStella](https://github.com/Sterophonick/PocketStella) | GBA Atari 2600 emulator | GPLv2 according to GitHub metadata | GBA compatibility/performance/PogoShell/GBA port techniques |
| [wavemotion-dave/StellaDS](https://github.com/wavemotion-dave/StellaDS) | Nintendo DS Atari 2600 emulator, Phoenix edition | NO_LICENSE metadata/unverified | CPU/TIA/audio/optimization research; not usable without separate license verification |
| [stella-emu/stella](https://github.com/stella-emu/stella) | Reference Atari 2600 emulator | GPLv2 according to GitHub metadata | Accuracy and reference behavior |
| [JetSetIlly/Atari2600-Flappy-Bird](https://github.com/JetSetIlly/Atari2600-Flappy-Bird) | Open-source Atari 2600 Flappy Bird source/game | BSD-2-Clause according to GitHub metadata | Compatibility test ROM candidate |

---

## 5. Hardware/software knowledge base

Each fact needs a source or evidence. Separate verified from unverified.

### VERIFIED
- Repository archive `a26.zip` contains `a26.gba`, open-source test source, test JSON/text, `flappy.a26`, and `repositories.txt` (source: archive listing and extraction).
- Prepared source reproduces the archived plugin SHA-256 `14d28e63...f4500` with devkitARM 16.1.0 (source: Docker build and `sha256sum`).
- Prepared plugin source is GPLv3-or-later adapters/menu with upstream GPL legacy code; `COPYING` and `NOTICE.txt` are retained (source: archive).
- Stock Ubuntu mGBA 0.10.2 is available at `/usr/games/mgba` and Xvfb is installed (source: apt).
- GitHub metadata identifies MikeDX/2600_gba and PocketStella as GPL-2.0, Stella as GPL-2.0, StellaDS as NO_LICENSE, and Flappy Bird BSD-2-Clause (source: GitHub API metadata; license files should be independently read before copying).
- Existing prepared plugin supports 2K/4K/8K F8/16K F6 only; no TIA audio implementation is present (source: `atari2600-test.txt`, source search).
- PogoShell argument block is fixed at EWRAM `0x0203fbf8` and ROM handoff validates a known ABI marker (source: `startup.S`, `files.c`).
- A hardware SuperFlash 822 trial and compatibility claims beyond the diagnostic are unqualified (source: `atari2600-test.txt`).
- A standalone `A26_FIXTURE` GBA image embeds `archive-inputs/flappy.a26` and survives 3 seconds under stock Ubuntu mGBA 0.10.2/Xvfb (source: `scripts/test-baseline.sh`).
- Public PocketNES/GoombaColor source confirms PogoShell launch at `0x0203FBFC` and tail at `0x0203FBF8`, plus ROM filename parsing from `0x0203FC08` (source: public GPL/Github references; license verify per-file before reuse).
- EZ-FLASH Omega kernel public metadata is Apache-2.0 (source: GitHub API); it is a kernel reference only, not evidence of physical cartridge behavior.

### UNVERIFIED
- PogoShell ABI details, return stub layout, and save memory behavior beyond the source's fixed argument block (how to verify: obtain PogoShell/plugin source or a real firmware experiment).
- SuperCard/EZ-Flash Omega harness behavior and kernel remapping; no existing harness patches were found in this repository (how to verify: build patched mGBA and compare with vendor/kernel evidence).
- Real-hardware speed and `60 fps` feasibility (how to verify: baseline timer/frame instrumentation and physical GBA measurement).
- `flappy.a26` provenance and license: archive does not include a notice for this file (how to verify: identify upstream source/license; no commercial ROM).

Topics to cover: PogoShell plugin ABI (load, launch, ROM handoff, save/state memory, return to shell); EZ-Flash Omega and SuperCard/SuperFlash behavior (SD/FAT access, save types, waitstates); memory layout used by the plugin; timing budgets (cycles per scanline/frame); TIA audio plan (Direct Sound/PSG).

---

## 6. Task board

### In progress
- **Task:** Baseline and harness audit
- **Exact step:** Normal plugin and standalone fixture builds pass; next replace stock fixture seam with a PogoShell/mGBA harness and capture hashes.
- **Files touched:** `AGENTS.md`, `PROGRESS.md`, `HANDOFF.md`, `.devcontainer/*`, `scripts/*`, `archive-inputs/*`
- **Uncommitted changes:** yes
- **Branch:** main
- **Resume command:** `bash scripts/build-plugin.sh && bash scripts/run-mgba.sh archive-inputs/test-source/atari2600/prototype.gba 5`

### Next up (ordered)
1. Onboard and audit: unpack `a26.zip`, map repo, reproducible Codespace build.
2. Test harnesses: finish/verify mGBA SuperCard and EZ-Flash harnesses; headless run with screenshots/frame hashes.
3. Baseline measurements (performance + screenshot/frame-hash), before any emulation changes.
4. Performance: reach full speed (60 fps NTSC or documented frame-skip).
5. Sound: TIA audio via Direct Sound and/or PSG.
6. Menu/UI parity with Goomba (font first, then layout, navigation, options, save-state conventions).
7. Compatibility and polish: bankswitching, PAL/NTSC, controllers, per-game settings, saves.

### Backlog
- Headless screenshot/frame-hash capture
- PogoShell return and ROM ABI harness in upstream mGBA

### Done
| Date | Task | Commit | Notes |
|------|------|--------|-------|
| 2026-11-24 | Initial audit and reproducible build baseline | cf5e58f | Extracted archive, created handoff, Docker build reproduces plugin SHA-256 |

---

## 7. Performance log

| Date | Commit | Test ROM | Metric | Method | Result |
|------|--------|----------|--------|--------|--------|
| | | | | | |

---

## 8. Test status

| Test/harness | How to run | Status | Notes |
|--------------|-----------|--------|-------|
| Reproducible plugin build | `bash scripts/build-plugin.sh` | PASS | SHA-256 matches `archive-inputs/a26.gba` |
| Stock mGBA standalone fixture | `bash scripts/test-baseline.sh` | PASS | Fixture survived 3 seconds under stock mGBA 0.10.2/Xvfb |
| Stock mGBA plugin launch | `bash scripts/run-mgba.sh <plugin> <seconds>` | NOT VALID | Plugin rejected as incompatible because PogoShell ABI is absent |
| PogoShell ABI harness | `<TODO>` | NOT STARTED | no patched harness found or built |

### Known harness-vs-hardware differences
- Stock mGBA cannot yet validate PogoShell ABI, firmware cartridge remapping, or physical Exit; `atari2600-test.json` uses a generated ROM and a return marker stub in stock SDL mGBA.

---

## 9. Known issues and decisions

### Issues
- <bug, repro steps, suspected cause>

### Dead ends (do not repeat)
- Direct devkitPro apt installer endpoint at `apt.devkitpro.org` returned HTTP 403 in this environment; use official Docker image or a host-specific installer.

### Design decisions
- Official `devkitpro/devkitarm:latest` Docker image is the default build path because it produced the exact archived plugin hash and avoids broken host bootstrap.
- Keep extracted archive assets under `archive-inputs/` until source layout is stabilized; do not add build outputs or commercial ROM payloads to Git.
- Use a standalone fixture only for core smoke tests; never treat stock-mGBA PogoShell simulation as proof of card firmware behavior.

### Licensing notes
- Archive prepared source retains GPLv3; legacy upstream code is GNU GPL as stated in source/CREDITS without a numbered version in some files.
- MikeDX/PocketStella/Stella GitHub metadata says GPL-2.0; verify license files before reuse. StellaDS metadata is NO_LICENSE and remains excluded pending verification. Flappy Bird metadata says BSD-2-Clause; verify upstream contents/license before use.
- PocketNES and GoombaColor PogoShell ABI usage was studied from public sources; no code was copied into this repository.

---

## 10. Open questions for the owner

1. <question> (blocked tasks: <list>; working on <other task> meanwhile)

---

## 11. Session log

Short dated entries. Trim old entries periodically.

- 2026-11-24: Read AGENTS/PROGRESS; extracted `a26.zip`; inventoried source/test assets and reference repositories; created HANDOFF and devcontainer scripts; verified Docker devkitARM 16.1.0 reproduces archived SHA-256 `14d28e63...f4500`; installed stock mGBA 0.10.2/Xvfb. Next: implement native baseline checks and PogoShell/mGBA harness.
