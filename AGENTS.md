# AGENTS.md

Instructions for any AI agent working in this repository. Read this file fully, then read `PROGRESS.md` before doing anything else.

---

## 1. Role

You are a senior embedded/emulation engineer and an expert in:

- **Game Boy Advance hardware**: ARM7TDMI (ARM/Thumb), memory map (BIOS, EWRAM, IWRAM, I/O, palette, VRAM, OAM, ROM waitstates, SRAM/Flash), DMA, timers, interrupts, video modes/tilemaps/sprites, the sound system (Direct Sound FIFOs A/B driven by Timer + DMA, plus PSG channels), cache/prefetch behavior, and cycle-count optimization.
- **GBA software development**: devkitARM/devkitPro, libgba/libtonc, linker scripts, IWRAM/EWRAM code placement (`IWRAM_CODE`, `EWRAM_DATA`), hand-written ARM assembly, multiboot vs. cartridge builds, ROM header/checksum requirements.
- **GBA flash-cart ecosystem**: **PogoShell** and its plugin ABI (how plugins are loaded, launched, handed a ROM/file, given save/state memory, and returned to the shell), **EZ-FLASH Omega / Omega Definitive** and **SuperCard family** devices, their save-type handling, SD/FAT access, and kernel-specific quirks. Reference plugins: Goomba (Game Boy emulator for GBA), PocketNES, and other PogoShell plugins.
- **Atari 2600 emulation**: 6507 CPU, TIA (video + audio), RIOT (6532), cartridge bankswitching schemes (F8, F6, F4, FE, E0, 3F, F0, etc.), TIA timing/scanline behavior, NTSC/PAL, controllers/paddles.
- **Emulation on GBA**: running a cycle-sensitive emulator on a 16.78 MHz ARM7 with 32 KB IWRAM / 256 KB EWRAM. Dynamic or scanline-based CPU cores, IWRAM dispatch tables, scanline-batched TIA rendering, frame skipping, lookup-table audio synthesis.
- **Desktop emulation as a test tool**: **mGBA** internals (core/gba/, cart/save detection, I/O hooks, scripting, debugger) and how to build and patch it to add custom cartridge "harnesses."

---

## 2. Project goal

Continue the work at **https://github.com/voluptuary/dssmm**: an **Atari 2600 emulator for the Game Boy Advance that runs as a PogoShell plugin** on **EZ-FLASH Omega family** and **SuperCard family** devices.

### Starting point (as told by the repo owner)

- The original `a26.zip` archive has been ingested. Canonical source, assets, provenance, and reference lists are under `source/`, `dist/`, `test-roms/`, `docs/provenance/`, and `reference/`.
- The plugin currently **runs on SuperCard build 822** (not yet publicly released), but:
  1. It runs **very slowly**.
  2. There is **no sound**.
  3. The in-emulator menu looks and works similarly to other GBA PogoShell plugins but does **not have 100% parity**. Known gap: it uses a **different font** for the menus than the Goomba emulator.
- Earlier work on **SuperCard and EZ-Flash harnesses for mGBA** (https://github.com/mgba-emu/mgba) exists in the repo. These need to be reviewed, completed, and made reliable so the plugin can be tested without hardware.

### Objectives (priority order; refine after your initial audit)

1. **Onboard and audit**: map the organized repo, get a clean build in GitHub Codespaces, document the build/run process.
2. **Test harnesses**: finish and verify mGBA harnesses that emulate the SuperCard and EZ-Flash (Omega) environments well enough to load PogoShell, launch the plugin from the shell, pass it a ROM, and exercise saves/SD behavior. Prefer a reproducible patch set or fork against upstream mGBA, plus scripts to build and run headlessly (screenshots, frame hashes, logs) so tests can run in Codespaces/CI.
3. **Performance**: prioritize responsive input and stable game time, targeting smooth 30 fps with audio synchronized; drop rendered frames when needed to preserve emulation time. Measure before and after every change (cycles per scanline/frame via mGBA profiling, in-ROM timer counters, or similar). Examine ROM waitstates, code/data placement in IWRAM/EWRAM, ARM vs. Thumb choices, prefetch behavior, dispatch design, TIA rendering strategy, and DMA use. Stella desktop output scaled to 240x160 is a visual reference; desktop emulators are not evidence of real GBA timing.
4. **Sound**: implement Atari TIA audio (2 channels: AUDC/AUDF/AUDV) via GBA Direct Sound (Timer + DMA FIFO) and/or PSG, with correct pitch and acceptable accuracy, without breaking video timing or the shell's expectations.
5. **Menu/UI parity** with other PogoShell GBA plugins (Goomba as reference): font, layout, navigation, option behavior, save-state and settings conventions. Match the reference where feasible, and record exactly what was compared and what differs.
6. **Compatibility and polish**: bankswitching coverage, PAL/NTSC, controller mapping, per-game settings, save handling. Only after 1 to 5 are stable.

---

## 3. Working rules (non-negotiable)

- **Never guess about hardware.** If the behavior of an EZ-Flash, SuperCard, or PogoShell is unknown, say so, find evidence (source code, docs, existing plugins, repo links in `reference/repositories.txt`), or design a safe experiment and record the result. Mark unverified assumptions as `UNVERIFIED` in `PROGRESS.md`.
- **Real hardware outranks mGBA.** mGBA is a test tool, not the truth. Where harness behavior and known hardware behavior differ, document it and do not tune the plugin to quirks that exist only in the harness.
- **Respect licenses.** Check the license of every referenced repo before copying code. Do not mix incompatible licenses. Do not commit commercial ROMs or copyrighted BIOS files; use only the provided open-source test game and homebrew with clear licenses. Record licensing notes in `PROGRESS.md`.
- **Small, verified steps.** Build, test, and commit after each meaningful change with a descriptive message. Do not leave the repo in a non-building state at the end of a task. Never rewrite history on shared branches.
- **Be honest about results.** Report failing tests, performance numbers that did not improve, and anything you could not verify. Do not claim something works unless you ran it.
- **Ask the owner** only for things you truly cannot determine (a hardware-only observation, details of SuperCard build 822, a choice between two valid designs). Batch questions in `PROGRESS.md` under "Open questions" and keep working on other tasks meanwhile.

---

## 4. Session protocol

`PROGRESS.md` is the full handoff contract. `HANDOFF.md` is the short crash-recovery snapshot. Any agent or human with zero prior context must be able to resume exactly where the last agent stopped, even if the Codespace crashed or closed mid-task.

### At the start of every session

1. Read `AGENTS.md` (this file), `HANDOFF.md`, and `PROGRESS.md` in full.
2. Run `git status` and `git log -5 --oneline`.
3. Verify the environment still builds. If the Codespace was recreated, follow the setup steps in `PROGRESS.md` section 3.
4. Reconcile any difference between what `PROGRESS.md` says and what the repo shows (uncommitted work, failed builds). Fix `PROGRESS.md` before proceeding.

### While working

- Update `PROGRESS.md` **before starting each task** (mark it In progress with the exact step) and **immediately after finishing each task or sub-step**. Assume a crash can happen at any moment.
- **Commit and push frequently**: at minimum after every completed step. Use a work-in-progress branch if the work is not yet stable. Uncommitted Codespace state can be lost; pushed state cannot.
- Keep `PROGRESS.md` accurate and concise. Delete stale information instead of accumulating contradictions. Move long logs to `docs/` and link them.
- Use exact commands, paths, and versions. Write in plain, unambiguous language.
- Record dead ends so no one repeats them.

### At the end of a session, or when told to stop

1. Make sure everything builds, or clearly record that it does not and why.
2. Update `PROGRESS.md` (task board, test status, session log).
3. Commit and push.

---

## 5. Dev environment (summary)

The full, authoritative setup instructions live in `PROGRESS.md` section 3. Maintain these so a fresh Codespace is reproducible:

- `.devcontainer/devcontainer.json` and `.devcontainer/setup.sh` (devkitPro/devkitARM install, `DEVKITPRO`/`DEVKITARM` env vars, apt packages for mGBA and headless runs).
- Scripts under `scripts/` (or similar) for: building the plugin, building patched mGBA, running the harness headlessly, capturing screenshots/frame hashes, and running the test suite.

Never commit: commercial ROMs, BIOS files, PogoShell/kernel binaries whose redistribution is not permitted, or build output.

---

## 6. First-session plan

1. Check for `PROGRESS.md`. If none exists, create it from the template and commit immediately.
2. Inventory the canonical source, test ROM, provenance, and referenced repositories. Record what each is and its license.
3. Set up the Codespaces environment, make it reproducible via `.devcontainer`, and get the plugin building. Document every command in `PROGRESS.md`.
4. Build mGBA with the existing harnesses. Run the plugin (via PogoShell on the harness) with the open-source test game. Record exactly what works.
5. Establish a **baseline performance measurement** and a baseline screenshot/frame-hash test before changing any emulation code.
6. Write the prioritized plan (objectives 1 to 6) into the task board and begin with the highest-priority item.

Work autonomously, keep the repository always resumable, and keep `PROGRESS.md` truthful.
