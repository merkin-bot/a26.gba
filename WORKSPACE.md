# Build the workspace offline from a repository checkout

The primary setup path is repository-only after checkout: it restores exact-pinned mGBA, DSSMM, PocketNES, and devkitARM inputs from `vendor/offline/`. It makes no network requests. On a minimal Ubuntu 24.04 host, install the required packages from the repository's local `.deb` archive, then build and run the baseline:

    scripts/install-offline-host-tools.sh
    python3 scripts/setup-offline-workspace.py
    source work/offline/devkitpro/devkitarm.sh
    bash scripts/offline-baseline.sh

`offline-baseline.sh` performs source restore, local libgba/Maxmod builds, plugin and fixture builds, PogoShell ABI check, FAT image check, stock and in-tree mGBA boots, all model/bus tests, the SuperCard mGBA runner build, the pinned SuperFW source/fixture suite, and all three EZ source-kernel/native-fixture/image lanes. It uses only repository inputs after host packages are available. Set `OFFLINE_SKIP_EXTENDED=1` only when tracing the core baseline under `strace`, because LeakSanitizer cannot run under ptrace; run the full command without that variable for the required checks.

## Platform baseline and scope

A repository cannot boot a computer or supply its kernel/libc. The supported base is x86_64 Ubuntu 24.04 with apt/dpkg, sudo, a shell, and libc/kernel already present. `scripts/install-offline-host-tools.sh` uses only `vendor/offline/packages` and disables outside apt sources for that command; it includes GCC/CMake/Ninja, Python, Unicorn/lcov, FAT32 and exFAT tools, mGBA, Stella, Xvfb, xdotool, and Pillow. The repository therefore supplies the full workspace software after the OS checkout, but is not a self-contained operating system.

Do not use an external apt source, `pip`, upstream `git clone`, or a container image pull for offline qualification. Generated binaries, SD images, and build directories stay ignored. Physical cards are unavailable in the standard Codespaces environment.

## Offline bundle contents

`vendor/offline/README.md` records the exact revisions, licenses/provenance, image digest, and SHA-256 values. The setup script verifies the manifest before restoring ignored checkouts and `work/offline/devkitpro/`; it builds pinned libgba and Maxmod from their local source bundles. The devkitARM archive is extracted from the official immutable devkitPro image and includes GCC 16.1.0 and `gbafix`; no image runtime is used. See the bundle README before updating any input.

## Desktop Stella

Stella, stock mGBA, Xvfb, xdotool, and Python Pillow are in the repository's offline Ubuntu package set. After `scripts/install-offline-host-tools.sh`, `scripts/capture-stella-reference.sh` can produce the documented native Atari comparison without contacting external services. See the Stella section for the crop and scaling method. Stella remains a visual diagnostic, not GBA timing evidence.

## Build and check

Run from the repository root:

    bash scripts/build-plugin.sh
    python3 scripts/test-pogo-abi.py
    bash scripts/verify-harness.sh
    bash scripts/build-fixture.sh
    bash scripts/test-baseline.sh
    cmake -S harnesses -B build/harness-cmake -G Ninja
    cmake --build build/harness-cmake --target supercard-model-test mgba-core-smoke supercard-mgba-runner ezflash-mgba-trace-smoke ezflash-sd-model-test ezflash-mgba-sd-smoke ezflash-mgba-runner -j2
    build/harness-cmake/supercard-model-test
    build/harness-cmake/mgba-core-smoke
    build/harness-cmake/ezflash-mgba-trace-smoke
    build/harness-cmake/ezflash-sd-model-test
    build/harness-cmake/ezflash-mgba-sd-smoke

The baseline builds the plugin, checks the PogoShell ABI, creates the virtual FAT image, builds standalone and card-handoff fixtures, and boots them under stock mGBA/Xvfb. Some checks repeat; the baseline is the broad initial verification.

`scripts/build-plugin.sh` accepts `A26_OPT_LEVEL=0|1|2|3|s`; it defaults to `3`, which was measured about 1.2% faster in the current Chis mGBA fixed-state replay while preserving identical capture output. `A26_OPT_LEVEL=2 bash scripts/build-plugin.sh` reproduces the prior compiler setting. Performance claims still require the repeatable Atari replay and valid hardware evidence for GBA claims.

For SuperCard diagnostics, use the matching DSSMM kernel from the pinned source and follow `harnesses/README.md` plus `PROGRESS.md`. Run `make clean` before switching SuperFW `BOARD` among `chis`, `sd`, and `lite` because generated intermediate names overlap. Mark card behavior `UNVERIFIED` where evidence is absent.

### Build and run the three EZ release-lane mGBA paths

Run the complete reproducible EZ sequence with:

    scripts/verify-ezflash-mgba.sh

It runs the three existing five-case native launch suites at their matching tags, clean-builds each matching source kernel, creates separate 64 MiB images with kernel/plugin/ROM, runs 300 mGBA GBA frames through the synthetic SD adapter, checks image integrity and embedded hashes, then restores the ignored DSSMM checkout to the pinned baseline. These are software-model results only. To inspect or repeat the steps individually, use the commands below.

Use the three exact source tags recorded in `harnesses/cards.json`. These commands build source kernels into the ignored DSSMM checkout; no binary is committed. `make clean` avoids stale objects when moving among lane snapshots:

    source work/offline/devkitpro/devkitarm.sh
    git -C harnesses/third_party/dssmm checkout v8.4.1
    make -C harnesses/third_party/dssmm/ezfomegade clean
    make -C harnesses/third_party/dssmm/ezfomegade -j2
    git -C harnesses/third_party/dssmm checkout v8.4.1-omega-test5
    make -C harnesses/third_party/dssmm/ezfomega clean
    make -C harnesses/third_party/dssmm/ezfomega -j2
    git -C harnesses/third_party/dssmm checkout v8.4.1-blue-test5
    make -C harnesses/third_party/dssmm/ezfblue clean
    make -C harnesses/third_party/dssmm/ezfblue -j2

Outputs are `ezfomegade/ezfomegade.gba`, `ezfomega/ezfomega.gba`, and `ezfblue/targets/blue/ezbluekernel-development.gba`. Create separate fresh FAT32 images and run each matching kernel/image pair:

    bash scripts/make-sd-image.sh work/ez-de-sd.img 64
    bash scripts/make-sd-image.sh work/ez-omega-sd.img 64
    bash scripts/make-sd-image.sh work/ez-blue-sd.img 64
    build/harness-cmake/ezflash-mgba-runner harnesses/third_party/dssmm/ezfomegade/ezfomegade.gba 300 work/ez-de-sd.img work/ez-de.ppm
    build/harness-cmake/ezflash-mgba-runner harnesses/third_party/dssmm/ezfomega/ezfomega.gba 300 work/ez-omega-sd.img work/ez-omega.ppm
    build/harness-cmake/ezflash-mgba-runner harnesses/third_party/dssmm/ezfblue/targets/blue/ezbluekernel-development.gba 300 work/ez-blue-sd.img work/ez-blue.ppm
    fsck.fat -n work/ez-de-sd.img
    fsck.fat -n work/ez-omega-sd.img
    fsck.fat -n work/ez-blue-sd.img

`scripts/make-sd-image.sh` creates `SYSTEM/PLUG`, `SYSTEM/SAVER`, and `ROMS`, then copies the Atari plugin/test ROM. The matching kernel is passed directly as the runner's cartridge input; it is not copied to the SD image. EZ-FLASH DSSMM uses `/SYSTEM/KERNELS` for optional theme-style `.bin` files, not for the active kernel; `EZ_THEME_STYLE_BIN=<file.bin>` stages a style only when a scenario explicitly tests it. To add a known-working PogoShell control plugin and a ROM, set all four variables: `CONTROL_PLUGIN_GBA`, `CONTROL_PLUGIN_EXT` (for example `nes`), `CONTROL_ROM`, and `CONTROL_ROM_NAME` (for example `snailmaze.nes`). These are staged as `SYSTEM/PLUG/nes.gba` and `ROMS/snailmaze.nes`; the control files remain local/ignored. `ezflash-mgba-runner` reports sector command/read/write totals and a 240x160 framebuffer hash; optional comma-separated events accept a one-frame `frame:mask` tap or inclusive `first-last:mask` held range (GBA button bitmasks). With `A26_EZ_TRACE_PC=1`, per-frame output includes guest PC and last SD LBA/count/direction. `A26_EZ_TRACE_EWRAM=<aligned address>` prints two adjacent words for temporary source-instrumentation diagnosis. The model uses synthetic ready status zero and no card latency, while its callbacks invoke mGBA upstream callbacks to retain GBA-side access costs. A successful run only means the matching kernel executed its UI and storage code through this model. Check `PROGRESS.md` for current handoff limits; these runs do not prove physical card behavior or a real-time GBA timing result.

To qualify a newer kernel without changing the pinned source bundle, copy it into ignored `work/kernels/` and set the profile override when running `scripts/verify-ezflash-mgba.sh`: `EZ_DE_KERNEL_GBA=work/kernels/de.gba`, `EZ_OMEGA_KERNEL_GBA=work/kernels/omega.gba`, and/or `EZ_BLUE_KERNEL_GBA=work/kernels/blue.gba`. Set `EZFLASH_PROFILE` to `ezfomegade`, `ezfomega`, or `ezfblue` to run just that lane's five native fixture cases and image test. The script still runs native fixtures against pinned source revisions, then uses the supplied binary as the runner's cartridge input with an SD image that does not contain it. Record its upstream version, board/card revision, source URL or archive provenance, SHA-256, and test result in `harnesses/cards.json`; new kernels require requalification and do not inherit old evidence. For SuperCard profiles, pass the new kernel directly as the first argument to `supercard-mgba-runner`; it is not an SD card file. The source bundle can be advanced separately by updating the pinned commit and hash manifest.

### One shared six-profile exFAT image

The default cross-card qualification image is a 4 GiB exFAT image with 32 KiB clusters, the same filesystem setup used for every profile. The shared image contains `SYSTEM/PLUG/a26.gba`, `SYSTEM/PLUG/nes.gba`, an empty `SYSTEM/SAVER/`, `ROMS/flappy.a26`, and `ROMS/snailmaze.nes`. It does not contain a kernel or a theme-style payload. Each run receives a byte-for-byte copy of the same pristine image; the matching kernel is supplied to the runner separately. The script checks the pristine image hash and runs `fsck.exfat -n` before and after every profile:

    scripts/verify-six-profile-image.sh

It uses the matching EZ DE, Original OMEGA, BLUE, SuperCard SD, Lite, and Chis kernel artifacts produced by the documented source builds and the matching source-shaped mGBA model for each. Set `SIX_PROFILE_IMAGE_MIB=64` only for fast model debugging; use the default 4096 MiB for qualification. mGBA cannot establish whether physical cards accept the same image, and this check is not real GBA timing evidence. Current result and exact pristine hash are recorded in `harnesses/cards.json`.

The DSSMM SuperFW validation scripts use Unicorn and `lcov`. Ubuntu's `python3-unicorn` package is installed for the system Python; Codespaces may put another Python first on `PATH`. Run the complete SuperFW suite with:

    export PATH=/usr/bin:$PATH
    export ASAN_OPTIONS=detect_stack_use_after_return=0
    source work/offline/devkitpro/devkitarm.sh
    export CC=gcc
    bash harnesses/third_party/dssmm/superfw/scripts/verify.sh

The system Python path makes Unicorn available. ASan's default fake stack is incompatible with the test's GCC nested-function trampoline, so the setting disables stack-use-after-return instrumentation for that fixture only. The `CC=gcc` override is for the final host C/ASan coverage tests; SuperFW firmware Makefiles explicitly select `arm-none-eabi-gcc`. Do not apply either workaround to the GBA/card model or firmware build flags.

## Stella accuracy and performance references

Stella is a desktop diagnostic and visual reference. Its timing and host CPU use are not GBA performance evidence. Use the same open-source ROM and synchronize game state/input before comparing a frame; note TV standard, Stella version, and frame or event used. The Atari logical image is 160x190, while the TIA playfield spans 40x192 bits/scanlines. Preserve the complete playfield when choosing vertical crop/sampling; each playfield bit covers four Atari pixels. CRT overscan typically masks about 10–12 Atari pixels on each side. The capture defaults to a 12-pixel crop per side and accepts a per-ROM crop override, including zero. Inspect each game before cropping because scores and gameplay can use those edges. Do not shrink the entire Stella desktop window. `scripts/capture-stella-reference.sh` runs Stella with software video under Xvfb, takes a native snapshot, reduces Stella's doubled horizontal sampling to 160 Atari pixels, selects a centered 160x192 TIA viewport, applies the chosen horizontal crop, then maps it with nearest-neighbor sampling to exactly 240x160 pixels:

    scripts/capture-stella-reference.sh test-roms/flappy.a26 work/stella/flappy-240x160.png 12

The capture disables VSync and audio for headless operation. It samples shortly after startup; for state-sensitive comparisons, make the equivalent game state reproducible and adjust the capture delay in the script or use Stella's GUI and F12 snapshot. Its 160x192 viewport retains all TIA playfield scanlines; vertical resampling to 160 GBA rows is unavoidable, while horizontal nearest-neighbor expansion preserves each remaining source pixel. Stella's unscaled PNG and the GBA framebuffer use different render paths, so the 240x160 output is a size-matched visual reference, not a proof of identical crop/scaling or hardware accuracy. Record the per-ROM crop value beside each image and keep captured images under ignored `work/`.

For host-only speed diagnostics, run the same ROM in Stella at normal speed (`-speed 1`) or maximum-speed mode (`-turbo 1`) and record the Stella version, ROM hash, duration, and host load. These measurements help compare desktop reference runs and diagnose game-state progression; do not use them to claim GBA frame rate or native Atari timing. Use real GBA hardware or a validated hardware timing method for those claims.

## Additional Atari homebrew test ROMs

The optional catalog at `https://github.com/retrobrews/atari2600-games` was checked at commit `0e16f397027ebb5d3be204c90efff169419ba5f8`. Its ROMs use `.bin`, so stage an individual game with the plugin's expected `.a26` extension:

    git clone https://github.com/retrobrews/atari2600-games work/atari2600-games
    git -C work/atari2600-games checkout 0e16f397027ebb5d3be204c90efff169419ba5f8
    scripts/stage-homebrew-rom.sh work/atari2600-games flappy_the_duck

This copies `flappy_the_duck.bin` to ignored `work/test-roms/flappy_the_duck.a26` and prints its SHA-256. Read the matching `.txt` before use. The repository README says redistribution is approved only on the RetroBrew site/project; individual game rights vary. Do not commit or redistribute these ROMs without the rights holder's permission. See `docs/provenance/atari2600-homebrew-roms.md`.

## PocketNES control plugin and NES homebrew

Restore the pinned local-only reference checkouts and build PocketNES with the host devkitARM toolchain:

    python3 scripts/setup-offline-workspace.py
    source work/offline/devkitpro/devkitarm.sh
    make -C work/PocketNES -j2
    mkdir -p work/pocketnes-control
    cp work/PocketNES/pocketnes.gba work/pocketnes-control/nes.gba


The PocketNES repository has no top-level license file; build locally and do not redistribute its output from this workspace. RetroBrew's catalog README limits ROM distribution approval to its own site/project. Inspect `snailmaze.txt` or the chosen game's metadata and keep ROMs under ignored `work/`. See `docs/provenance/pocketnes-control.md` for checked hashes and the currently passing Chis launch result. To build a control image:

    CONTROL_PLUGIN_GBA=work/pocketnes-control/nes.gba CONTROL_PLUGIN_EXT=nes CONTROL_ROM=work/retrobrews-nes-games/snailmaze.nes CONTROL_ROM_NAME=snailmaze.nes bash scripts/make-sd-image.sh work/chis-pocketnes-sd.img 64
    fsck.fat -n work/chis-pocketnes-sd.img

For the source-linked Chis Pogo plugin loader/FatFs/ABI control fixture (requires the pinned DSSMM Chis `firmware.ewram.elf`, devkitARM, and system Python Unicorn):

    mkdir -p work/pocketnes-control
    cp work/PocketNES/pocketnes.gba work/pocketnes-control/nes.gba
    source work/offline/devkitpro/devkitarm.sh
    PATH=/usr/bin:$PATH python3 harnesses/third_party/dssmm/superfw/scripts/test-native-plugins-arm.py --pack work/pocketnes-control --extension nes --game-bytes 40976

The fixture uses generated same-size game bytes; use the full Chis mGBA kernel/image runner for an actual SD ROM launch. The synthetic EZ DE control now reaches the PocketNES/Snail Maze screen; `docs/provenance/pocketnes-control.md` records its 2,500-frame result and adapter limits. Repeat with the matching Original OMEGA and BLUE kernels before treating the EZ lanes as qualified. Physical card behavior remains UNVERIFIED.

## Recovery notes

- Regenerate ignored `source/atari2600/prototype.gba` with `bash scripts/build-plugin.sh`.
- Regenerate `work/a26-sd.img` and related artifacts with `bash scripts/verify-harness.sh`.
- If CMake cannot find mGBA, run `python3 scripts/setup-offline-workspace.py`; it restores the pinned source from the repository bundle.
- `scripts/build-plugin.sh`, `scripts/build-fixture.sh`, and `scripts/build-card-fixture.py` call host devkitARM directly; none require Docker.
- `HANDOFF.md` has the short checkpoint. `PROGRESS.md` has current harness status, evidence, test results, and next task.
