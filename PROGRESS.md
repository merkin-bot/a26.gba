# Project state and next actions

**Updated:** 2026-11-25 UTC
**Branch:** main
**Latest baseline checks:** completed in this session on Ubuntu 24.04.5 with devkitARM GCC 16.1.0. See §4 for commands/results. Plugin SHA-256: `cc868252fb113cd4160b8c6677d39cfe076056a4422f262b0975c99f5fb21de7`.

This is the source of truth for current status, exact next work, evidence, and test outcomes. Project requirements are in `AGENTS.md`; session kickoff is in `HANDOFF.md`; environment setup is in `WORKSPACE.md`.

## 1. Current task and next action

**Current task:** Complete the six-profile harness qualification using the reproducible host workspace and preserve the hardware/card/storage layers as separate evidence.

**Next action:** Trace the generic `File error` reached after selecting the staged `.a26` through the EZ mGBA runner. The runner does execute frames while PC samples at `0x000001F8` (mGBA HLE BIOS wait), so this address is not evidence of a permanently stalled kernel. A controlled Down + sustained-A sequence enters the SD browser and `/ROMS`; selecting `flappy.a26` reaches `File error` (framebuffer hash `797715054E71B915`). Identify the failing FatFs/SD operation and whether plugin data staging needs a PSRAM model. Keep source fixture coverage distinct from end-to-end path coverage. Then close the six-profile software harness gate, retaining explicit Chis NOR/SRAM and physical-card unknowns. Do not start emulator timing/render/audio optimization until plugin selection/handoff coverage and evidence are complete.

**Workspace checkpoint:** The host setup and plugin baseline are now verified. The official Debian bootstrap returns HTTP 403, so `WORKSPACE.md` documents the working immutable devkitPro image extraction fallback. No container runtime was used.

## 2. End goal and required order

Acceptance target and engineering rules are defined in AGENTS.md. Do not optimize the plugin before the harness gate; validate emulator behavior against the evidence-backed hardware model.

### Ordered work

1. **Workspace/baseline:** complete the current task above.
2. **Harness gate:** finish evidence-backed mGBA paths for the six profiles in `harnesses/cards.json`. Cover, as separate layers, GBA hardware; each matching card/kernel/launch profile; and SD/block storage with kernel/plugin/test ROM. Reuse DSSMM fixtures rather than duplicate launcher/UI tests. Test launch, ROM handoff, saves/storage, and return where evidence supports them. Clearly label seams/mocks and unknowns. Never alter a hardware model to make the Atari plugin pass.
3. **Emulator timing/performance:** establish real GBA timing evidence and correct measured bottlenecks.
4. **TIA audio:** implement and measure synchronized sound without compromising game time/video or shell operation.
5. **Menu parity:** compare with Goomba and record exact matches/gaps.
6. **Compatibility:** bankswitching, PAL/NTSC, controls, settings, and saves.

## 3. Current repository and harness state

- Plugin source: `source/atari2600/`; experimental/reference binary: `dist/a26.gba`; open-source test ROM: `test-roms/flappy.a26`.
- All six card profiles, matching kernel references, and profile-specific qualification state: `harnesses/cards.json`.
- Harness layering, implemented seams, commands and limits: `harnesses/README.md` and `docs/provenance/`.
- Ignored clones expected by harness work: mGBA `431935616c0e216d650b25b2ea0c0acccd359038` (MPL-2.0); DSSMM SuperFW `37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4` (Apache-2.0). Restore/build them with `WORKSPACE.md` as needed.
- Generated plugin, fixture, SD images, build directories and third-party clones are ignored; never add them to Git.

### Verified harness results

- EZ DE, Original OMEGA, and BLUE source-pinned DSSMM native launch suites each pass five launch fixture cases. These mock storage/boot seams; they do not qualify physical card or SD behavior.
- EZ passive mGBA trace observes four-word channel-3 DMA reads and writes while forwarding upstream behavior. Traced/untraced hashes and DMA scheduling/wait-cycle fields match. This is an observer, not an EZ card or SD emulator.
- EZ synthetic model and mGBA callback adapter compile. Host and mGBA GBA-bus callback smokes pass the four-step unlock transcript, multi-sector reads, 32-bit transfer byte order, pre-command DMA write staging/persistence, configured ready status, and out-of-image rejection. ARM LDM/STM hooks call upstream first and then apply synthetic values. Source-built DE, Original OMEGA, and BLUE kernels ran 300 GBA frames against separate fresh 64 MiB FAT images containing the matching kernel, plugin, and test ROM. DE/BLUE each read 154 and wrote 30 sectors; Original OMEGA read 100 and wrote 30; model failures were zero. All rendered the same 240x160 initial menu frame (`3DB0B0392899E8FC`). PC tracing samples `0x000001F8` in mGBA HLE BIOS while frameCounter continues to advance; that is a wait point, not proof of a permanently stalled CPU. Plugin selection/handoff and shell return are not established. Physical timing/card effects remain `UNVERIFIED`.
- SuperFW Build769 Chis/SD/Lite clean builds, ARM checks, settings save/reload, native menu scenes, and Chis NOR carousel scenes passed under the pinned DSSMM commit. These are source/fixture checks; physical bus, SD, NOR, and board-revision behavior remains partly `UNVERIFIED`.
- The SuperCard mGBA diagnostic completed 2,000 GBA frames for each Build769 board using the synthetic model and separate fresh FAT images containing the matching kernel, plugin, and ROM. SD reported one model block and FNV64 `30C994B7E348742D`; Lite/Chis reported 60 blocks and 3,900 port reads, FNV64 `357015EFA2DF90AB`; first traced EWRAM execution was frame 47 for each. Fixed-total-frame hashes are not synchronized game-state comparisons and prove no physical-card fidelity, real GBA timing, game correctness, or audio. Chis NOR/SRAM remains unmodeled.
- Detailed test commands, result hashes, known harness/hardware differences, and source citations remain in `harnesses/README.md`, `harnesses/cards.json`, and `docs/provenance/`.

### Unverified and incomplete

- EZ initial-menu/storage path and basic SD-browser navigation are exercised through synthetic images. Controlled inputs enter `/ROMS`, but selecting the staged `.a26` produces a generic `File error`; plugin handoff and shell return are not established. Physical status/response timing remains `UNVERIFIED`.
- Chis NOR/SRAM model: available HDL and Build769 source differ; board revision correspondence is not established.
- Full hardware qualification for all six cards/kernels, including real saves and shell return.
- SuperCard Build 822 behavior beyond the owner's report that the plugin runs slowly and silently.
- Accurate native Atari game speed, input latency, smooth GBA frame pacing, and synchronized sound on real hardware. No valid measured baseline for those outcomes is recorded yet.

## 4. Latest checks and environment state

The Ubuntu 24.04.5 host has the documented dependencies and pinned ignored mGBA/DSSMM checkouts. `https://apt.devkitpro.org/install-devkitpro-pacman` returns HTTP 403. The working fallback extracts the immutable official image `devkitpro/devkitarm:20260610` (`sha256:116afba8df8453961de2936ffab20dd441edf4d682856c1ec8b0e53d7ed0bbf5`) to `/opt/devkitpro` with `skopeo` and `umoci`; it does not run Docker. This provides devkitARM GCC 16.1.0 and `gbafix`.

Current checks:

- PASS: `python3 scripts/test-pogo-abi.py`
- PASS: `bash scripts/build-plugin.sh`; plugin SHA-256 `cc868252fb113cd4160b8c6677d39cfe076056a4422f262b0975c99f5fb21de7`
- PASS: `bash scripts/test-baseline.sh` (FAT32 verified; plugin and homebrew ROM hashes checked; standalone mGBA fixture survived 3 seconds; plugin launch skipped because portable PogoShell ABI is absent)
- PASS: `bash scripts/verify-harness.sh`
- PASS: `bash scripts/build-fixture.sh`
- PASS: `cmake -S harnesses -B build/harness-cmake -G Ninja`
- PASS: `cmake --build build/harness-cmake --target supercard-model-test mgba-core-smoke supercard-mgba-runner ezflash-mgba-trace-smoke -j2`
- PASS: `build/harness-cmake/supercard-model-test`
- PASS: `build/harness-cmake/mgba-core-smoke`
- PASS: `build/harness-cmake/ezflash-mgba-trace-smoke`
- PASS: `cmake --build build/harness-cmake --target ezflash_mgba_sd ezflash-sd-model-test ezflash-mgba-trace-smoke -j2`
- PASS: `build/harness-cmake/ezflash-sd-model-test`
- PASS: `cmake --build build/harness-cmake --target ezflash_mgba_sd ezflash-sd-model-test ezflash-mgba-sd-smoke ezflash-mgba-trace-smoke -j2`
- PASS: `build/harness-cmake/ezflash-mgba-sd-smoke` (GBA bus callbacks, synthetic image read/write, equal upstream callback cycle counts, ARM LDM/STM data path; logger silenced for expected out-of-range ROM callbacks)
- PASS: Updated EZ adapter invokes mGBA upstream scalar callbacks before applying synthetic data; bus smoke confirms equal callback cycle counts and executes ARM `loadMultiple`/`storeMultiple` on synthetic aperture data.
- PASS: `cmake --build build/harness-cmake --target ezflash-mgba-runner -j2`
- PASS: `scripts/verify-ezflash-mgba.sh` (all 15 native launch fixture cases, all three release source builds, and fresh kernel/plugin/ROM images; checkout restored to DSSMM pin)
- PASS: Controlled DE input on a fresh synthetic image: Down + A enters the SD browser; a sustained A enters `/ROMS` and displays the staged file. The `0x000001F8` PC samples occur at mGBA HLE BIOS wait points while frameCounter continues, not a permanent CPU stall. Earlier single-frame input sequence did not reach the folder/file stage.
- PASS: Final `scripts/verify-ezflash-mgba.sh` rerun after tightening the four-write unlock transcript: all 15 native launch fixtures and all three clean source-kernel builds/images pass. DE/BLUE read 154/write 30 sectors, Original OMEGA read 100/write 30, zero failures, same menu hash; image hashes are recorded in `cards.json`. Runner ends at a frame-boundary BIOS wait as expected; separate controlled input reaches `/ROMS`, but selecting `.a26` produces a generic `File error`, so plugin handoff is not established.
- PASS: Three matching Build769 SuperCard kernel + separate image runs, each 2,000 mGBA GBA frames. Generated images included kernel, plugin, and ROM; see `cards.json` for results and hashes.
- PASS: `scripts/stage-homebrew-rom.sh work/atari2600-games-inspect flappy_the_duck`; copied to ignored `.a26` with unchanged SHA-256 `e6b0e6db04d459a12e3202d695ce015fe8f84a0e67f7d26c87cc086ab500cab0`.
- PASS: `scripts/capture-stella-reference.sh work/test-roms/flappy_the_duck.a26 work/stella/flappy-the-duck-240x160.png 12`; output 240x160. Visual inspection found no obvious useful side-edge content in this startup frame; 12-pixel crop retained.
- PASS: EZ DE (`v8.4.1`), OMEGA (`v8.4.1-omega-test5`), and BLUE (`v8.4.1-blue-test5`) native mGBA launch suites; five cases per lane, fixture seams mocked.
- PASS: `bash harnesses/third_party/dssmm/superfw/scripts/verify.sh` with environment from `WORKSPACE.md`; includes clean SD/Lite/Chis builds, ARM checks, native settings/menu/NOR mGBA scenes, and host tests. The final host coverage target requires `CC=gcc`; ASan nested-callback test requires `ASAN_OPTIONS=detect_stack_use_after_return=0`.
- PASS: Stella 6.7.1 capture `scripts/capture-stella-reference.sh test-roms/flappy.a26 work/stella/flappy-240x160.png 12`; produced a 240x160 PNG from the 320x228 native Stella snapshot, reducing horizontal double-sampling, retaining 192 playfield scanlines, cropping 12 side pixels, and scaling with nearest-neighbor sampling. The default startup state is not synchronized with an emulator frame.
- PASS: Separate 64 MiB SuperCard images pass `fsck.fat -n` and contain matching `SYSTEM/KERNELS/KERNEL.GBA`, plugin, and ROM. Hashes: SD `8a4d3861e263f698340a661e1c38bfc87fce1c5597633b71baf03e442e61f3b2`, Lite `041a776efe8fe91299808a4f00aecd2f3db9a882f9e66b6712e85006f143a53d`, Chis `af231ad09991c9ef925e2c2acf1bb4161208b49bf7222cba3bc020fd1825842f`.
- PASS: Separate 64 MiB EZ images pass `fsck.fat -n`; exact matching kernels, plugin, and ROM were extracted back out with `mtype` and hash-matched after the runs. Latest post-run image hashes after the stricter four-write unlock rerun: DE `3c0e8b428484a301daebf9040c467b0e3b705d07ed6ba460c497b484eec92735`, OMEGA `b9132f9b8f4fb668e1712a068312c3e2bbe7725856ee36fd16c6e03ce2d9386`, BLUE `5ef5520b7ece59ff3589bd63a2acda16b38c917e38513f704f90e2f088d42219`.

The previous devcontainer/Docker-in-Docker setup failed because the restricted Codespace could not start nested Docker (iptables/NAT permission denied). `.devcontainer/` remains removed. Build scripts use host devkitARM tools; `WORKSPACE.md` is authoritative.

## 5. Durable constraints

Follow the hardware fidelity, evidence, measurement, build, mGBA, board-switching, and owner code-use rules in `AGENTS.md`. Profile-specific unknowns and evidence are tracked in `harnesses/cards.json` and `docs/provenance/`.

## 6. Session record

- 2026-11-25: Owner clarified Stella output guidance: map Atari's 160x190 logical image and 40x192 TIA playfield to GBA 240x160, retain all playfield scanlines, and crop CRT overscan of about 10–12 pixels per horizontal side when those edges contain no important game information. The capture defaults to 12 pixels per side but allows per-ROM adjustment down to zero. This is documented in AGENTS.md, WORKSPACE.md, and the capture script.
- 2026-11-25: Restored the host workspace using devkitPro's pinned official image after the official Debian bootstrap returned HTTP 403. Plugin and broad baseline pass with GCC 16.1.0. Stella 6.7.1 headless 240x160 capture pass. Full pinned SuperFW verification pass with documented Ubuntu Python/ASan/host-CC environment. EZ launch fixtures pass for all three release tags. Built separate fresh SD images for all three SuperCard profiles and reran each matching Build769 kernel through the source-shaped mGBA runner. All read 60 blocks without load failures; outputs are not frame-synchronized and do not establish physical behavior. Next: EZ block-storage adapter qualification.
- 2026-11-25: Session kickoff: read AGENTS.md, HANDOFF.md, PROGRESS.md, and WORKSPACE.md in order; clean `main` matches `origin/main` at `1ec9fde`. Current host is Ubuntu 24.04.5, but devkitARM/gbafix and the documented baseline tools are absent. Workspace qualification started; see follow-up entry for install/build outcome.
- 2026-11-25: Added isolated EZ raw-image SD model, mGBA scalar/LDM/STM callback adapter, and a 300-frame source-kernel runner. Host and GBA-bus smokes pass, including equal upstream cycle counts and multiple transfers. Matching DE/OMEGA/BLUE builds ran against independent 64 MiB FAT images carrying kernel/plugin/ROM, with read/write activity and zero model failures. All render the same initial menu. PC tracing shows kernel code enters BIOS wait (`0x1F8`) and remains halted despite IE/IF/IME showing VBlank enabled; later key taps did not change the frame. Cause and plugin handoff/shell return remain unverified. Synthetic status zero has no physical timing meaning.
- 2026-11-25: Added RetroBrew's Atari homebrew catalog to `reference/repositories.txt`, pinned its inspected commit and documented its README's distribution restriction and per-game rights caveat. Added a local-only `.bin` to `.a26` staging helper; staged Flappy the Duck in ignored `work/` with original hash intact and captured a 240x160 Stella reference at 12-pixel side crop. The startup frame showed no obvious important side-edge content; crop choice remains per-ROM and must be revisited on gameplay frames.
- 2026-11-25: Installed the documented Ubuntu host dependencies with apt and restored mGBA `431935616c0e216d650b25b2ea0c0acccd359038` plus DSSMM `37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4` as ignored checkouts. CMake harness targets and three host smoke executables plus PogoShell ABI check pass. Plugin/full baseline remains blocked because official devkitPro Debian bootstrap returns HTTP 403; official GitHub release notes refer Debian users to the same script and do not document an Ubuntu alternative. No plugin optimization or hardware claim was made.
- 2026-11-25: Removed the failed devcontainer/Docker-in-Docker path. Build scripts use host devkitARM; `WORKSPACE.md` describes fresh unrestricted Codespaces setup. Host-side model/core/EZ trace smokes and ABI check pass. Plugin/FAT/full-baseline checks await a new workspace with devkitARM installed.
- 2026-11-25: Owner clarified the final quality bar: accurate native Atari speed, smooth frame pacing and synchronized sound comparable in feel to established GBC/NES/SMS emulators. Hardware-faithful harnesses must never be changed to accommodate the emulator. Owner authorizes GitHub repository code use under GPL-2.0 without separate permission requests.
- 2026-11-25: Consolidated cold-start roles across AGENTS.md, HANDOFF.md, PROGRESS.md, and WORKSPACE.md. No code or harness behavior changed.

- 2026-11-25: Tightened EZ model acceptance to the four source-observed unlock writes before control/sector commands. Re-ran all three lane fixture suites, source builds, and 300-frame kernel/image runs successfully; refreshed image hashes in `cards.json`. Model, mGBA bus, broad harness, plugin baseline, ABI, syntax, and JSON checks pass. Follow-up input experiment proves the BIOS `0x1F8` sample is a frame wait and successfully navigates into `/ROMS`; selecting `.a26` ends on generic `File error`, now the next harness investigation.

- 2026-11-25: Corrected the EZ runner interpretation using a fresh image and frame traces: PC `0x1F8` is a recurring HLE BIOS wait point while frameCounter advances. A Down + A sequence opens the SD browser; sustained A enters `/ROMS`. Selecting the staged `.a26` reaches `File error` (hash `797715054E71B915`), with the failing FAT/storage or staging step still undiagnosed.
