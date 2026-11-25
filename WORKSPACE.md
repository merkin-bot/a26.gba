# Build the workspace in a fresh Codespace

Use a regular Ubuntu 24.04 Codespace with the unrestricted host environment. This repository does not use a devcontainer or Docker. The previous Docker-in-Docker setup could not start inside the restricted Codespace (dockerd was denied iptables/NAT access), so `.devcontainer/` has been removed. Run these commands in the Codespace terminal; sudo is required for system packages and devkitPro.

## 1. Install host tools

    sudo apt-get update
    sudo apt-get install -y build-essential cmake ninja-build pkg-config curl git unzip zip python3 python3-pil python3-unicorn lcov libepoxy-dev libgl1-mesa-dev libsdl2-dev libqt5opengl5-dev qtbase5-dev libpng-dev libzip-dev zlib1g-dev libavcodec-dev libavformat-dev libavutil-dev libswresample-dev libswscale-dev libedit-dev libelf-dev mgba-sdl xvfb xdotool xauth stella dosfstools mtools skopeo umoci

These provide build tools, Python, stock mGBA, Xvfb, Stella, and FAT image tools. Codespaces does not provide USB access to physical cards.

## 2. Install devkitARM without Docker

Use the [official devkitPro pacman instructions](https://devkitpro.org/wiki/devkitPro_pacman) and Debian/Ubuntu bootstrap. Review the installer before running it:

    curl -fsSLo /tmp/install-devkitpro-pacman https://apt.devkitpro.org/install-devkitpro-pacman
    less /tmp/install-devkitpro-pacman
    chmod +x /tmp/install-devkitpro-pacman
    sudo /tmp/install-devkitpro-pacman
    sudo /opt/devkitpro/pacman/bin/dkp-pacman -Sy --needed gba-dev
    source /etc/profile.d/devkit-env.sh

The official installer provides the `devkit-env.sh` profile used to set `DEVKITPRO`, `DEVKITARM`, and tool paths in new login shells. If the profile is not present, inspect the installed files under `/etc/profile.d/` and set those variables to the installed devkitPro paths; do not hardcode a guessed compiler location.

Verify the tools:

    arm-none-eabi-gcc --version
    arm-none-eabi-as --version
    command -v gbafix
    command -v cmake ninja cc python3 /usr/games/mgba xvfb-run xdotool stella mkfs.vfat mcopy fsck.fat

If the official bootstrap URL returns HTTP 403, autonomously follow the current installation alternatives linked from devkitPro's Linux instructions; do not restore Docker-in-Docker or substitute an unrelated ARM compiler. The prior restricted Codespace received HTTP 403, so confirm access in the new unrestricted Codespace. Historical builds used devkitARM GCC 16.1.0 from `devkitpro/devkitarm:latest`. The pacman package may be newer; record compiler version and plugin hash in `PROGRESS.md`. Claim exact archived hash reproduction only if the result matches.

### Official toolchain image fallback when the Debian bootstrap is blocked

The official devkitPro GitHub release notes route Debian/Ubuntu users to the bootstrap URL above. If that URL is blocked by HTTP 403, a reproducible fallback is to extract the official devkitPro Linux image into the host filesystem. This uses `skopeo` and `umoci` to download and unpack the immutable image; it does not run Docker or a container. Pin the image digest below rather than `latest`:

    skopeo inspect docker://docker.io/devkitpro/devkitarm:20260610
    skopeo copy docker://docker.io/devkitpro/devkitarm@sha256:116afba8df8453961de2936ffab20dd441edf4d682856c1ec8b0e53d7ed0bbf5 oci:/tmp/devkitarm-oci:qualified
    sudo umoci unpack --image /tmp/devkitarm-oci:qualified /tmp/devkitarm-rootfs
    sudo mkdir -p /opt/devkitpro
    sudo cp -a /tmp/devkitarm-rootfs/rootfs/opt/devkitpro/. /opt/devkitpro/
    source /opt/devkitpro/devkitarm.sh

The pinned image reports devkitARM GCC 16.1.0 and includes `gbafix`. Verify `arm-none-eabi-gcc --version`, `arm-none-eabi-as --version`, `command -v gbafix`, and the plugin build. `gbafix --help` is not a valid presence check on this release: it returns status 255. The copied toolchain runs as host binaries; do not add the image or its extracted files to Git.

## 3. Restore ignored source checkouts

Clones and build output are intentionally ignored. Run from the repository root:

    mkdir -p harnesses/third_party
    if [ ! -d harnesses/third_party/mgba/.git ]; then git clone https://github.com/mgba-emu/mgba harnesses/third_party/mgba; fi
    git -C harnesses/third_party/mgba checkout 431935616c0e216d650b25b2ea0c0acccd359038
    if [ ! -d harnesses/third_party/dssmm/.git ]; then git clone https://github.com/voluptuary/dssmm harnesses/third_party/dssmm; fi
    git -C harnesses/third_party/dssmm checkout 37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4

mGBA is MPL-2.0; DSSMM SuperFW is Apache-2.0. Do not commit these ignored clones, proprietary kernel/BIOS files, generated ROMs, or SD images.

## 4. Build and check

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

The baseline builds the plugin, checks the PogoShell ABI, creates the virtual FAT image, builds a standalone fixture, and runs it under stock mGBA/Xvfb. Some checks repeat; the baseline is the broad initial verification.

For SuperCard diagnostics, use the matching DSSMM kernel from the pinned source and follow `harnesses/README.md` plus `PROGRESS.md`. Run `make clean` before switching SuperFW `BOARD` among `chis`, `sd`, and `lite` because generated intermediate names overlap. Mark card behavior `UNVERIFIED` where evidence is absent.

### Build and run the three EZ release-lane mGBA paths

Run the complete reproducible EZ sequence with:

    scripts/verify-ezflash-mgba.sh

It runs the three existing five-case native launch suites at their matching tags, clean-builds each matching source kernel, creates separate 64 MiB images with kernel/plugin/ROM, runs 300 mGBA GBA frames through the synthetic SD adapter, checks image integrity and embedded hashes, then restores the ignored DSSMM checkout to the pinned baseline. These are software-model results only. To inspect or repeat the steps individually, use the commands below.

Use the three exact source tags recorded in `harnesses/cards.json`. These commands build source kernels into the ignored DSSMM checkout; no binary is committed. `make clean` avoids stale objects when moving among lane snapshots:

    source /opt/devkitpro/devkitarm.sh
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

    KERNEL_GBA=harnesses/third_party/dssmm/ezfomegade/ezfomegade.gba bash scripts/make-sd-image.sh work/ez-de-sd.img 64
    KERNEL_GBA=harnesses/third_party/dssmm/ezfomega/ezfomega.gba bash scripts/make-sd-image.sh work/ez-omega-sd.img 64
    KERNEL_GBA=harnesses/third_party/dssmm/ezfblue/targets/blue/ezbluekernel-development.gba bash scripts/make-sd-image.sh work/ez-blue-sd.img 64
    build/harness-cmake/ezflash-mgba-runner harnesses/third_party/dssmm/ezfomegade/ezfomegade.gba 300 work/ez-de-sd.img work/ez-de.ppm
    build/harness-cmake/ezflash-mgba-runner harnesses/third_party/dssmm/ezfomega/ezfomega.gba 300 work/ez-omega-sd.img work/ez-omega.ppm
    build/harness-cmake/ezflash-mgba-runner harnesses/third_party/dssmm/ezfblue/targets/blue/ezbluekernel-development.gba 300 work/ez-blue-sd.img work/ez-blue.ppm
    fsck.fat -n work/ez-de-sd.img
    fsck.fat -n work/ez-omega-sd.img
    fsck.fat -n work/ez-blue-sd.img

`KERNEL_GBA` also copies that exact profile kernel into `SYSTEM/KERNELS/KERNEL.GBA` on the raw image; the runner separately uses it as its cartridge input. `ezflash-mgba-runner` reports sector command/read/write totals and a 240x160 framebuffer hash; an optional comma-separated `frame:mask` argument injects GBA button masks. The model uses synthetic ready status zero and no card latency, while its callbacks invoke mGBA upstream callbacks to retain GBA-side access costs. A successful run only means the matching kernel executed its UI and storage code through this model. Check `PROGRESS.md` for current handoff limits; these runs do not prove physical card behavior or a real-time GBA timing result. After building, return the ignored DSSMM checkout to its pinned commit with `git -C harnesses/third_party/dssmm checkout 37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4`.

The DSSMM SuperFW validation scripts use Unicorn and `lcov`. Ubuntu's `python3-unicorn` package is installed for the system Python; Codespaces may put another Python first on `PATH`. Run the complete SuperFW suite with:

    export PATH=/usr/bin:$PATH
    export ASAN_OPTIONS=detect_stack_use_after_return=0
    source /opt/devkitpro/devkitarm.sh
    export CC=gcc
    bash harnesses/third_party/dssmm/superfw/scripts/verify.sh

The system Python path makes Unicorn available. ASan's default fake stack is incompatible with the test's GCC nested-function trampoline, so the setting disables stack-use-after-return instrumentation for that fixture only. The `CC=gcc` override is for the final host C/ASan coverage tests; SuperFW firmware Makefiles explicitly select `arm-none-eabi-gcc`. Do not apply either workaround to the GBA/card model or firmware build flags.

## 5. Desktop Stella accuracy and performance references

Stella is a desktop diagnostic and visual reference. Its timing and host CPU use are not GBA performance evidence. Use the same open-source ROM and synchronize game state/input before comparing a frame; note TV standard, Stella version, and frame or event used. The Atari logical image is 160x190, while the TIA playfield spans 40x192 bits/scanlines. Preserve the complete playfield when choosing vertical crop/sampling; each playfield bit covers four Atari pixels. CRT overscan typically masks about 10–12 Atari pixels on each side. The capture defaults to a 12-pixel crop per side and accepts a per-ROM crop override, including zero. Inspect each game before cropping because scores and gameplay can use those edges. Do not shrink the entire Stella desktop window. `scripts/capture-stella-reference.sh` runs Stella with software video under Xvfb, takes a native snapshot, reduces Stella's doubled horizontal sampling to 160 Atari pixels, selects a centered 160x192 TIA viewport, applies the chosen horizontal crop, then maps it with nearest-neighbor sampling to exactly 240x160 pixels:

    scripts/capture-stella-reference.sh test-roms/flappy.a26 work/stella/flappy-240x160.png 12

The capture disables VSync and audio for headless operation. It samples shortly after startup; for state-sensitive comparisons, make the equivalent game state reproducible and adjust the capture delay in the script or use Stella's GUI and F12 snapshot. Its 160x192 viewport retains all TIA playfield scanlines; vertical resampling to 160 GBA rows is unavoidable, while horizontal nearest-neighbor expansion preserves each remaining source pixel. Stella's unscaled PNG and the GBA framebuffer use different render paths, so the 240x160 output is a size-matched visual reference, not a proof of identical crop/scaling or hardware accuracy. Record the per-ROM crop value beside each image and keep captured images under ignored `work/`.

For host-only speed diagnostics, run the same ROM in Stella at normal speed (`-speed 1`) or maximum-speed mode (`-turbo 1`) and record the Stella version, ROM hash, duration, and host load. These measurements help compare desktop reference runs and diagnose game-state progression; do not use them to claim GBA frame rate or native Atari timing. Use real GBA hardware or a validated hardware timing method for those claims.

## 6. Additional Atari homebrew test ROMs

The optional catalog at `https://github.com/retrobrews/atari2600-games` was checked at commit `0e16f397027ebb5d3be204c90efff169419ba5f8`. Its ROMs use `.bin`, so stage an individual game with the plugin's expected `.a26` extension:

    git clone https://github.com/retrobrews/atari2600-games work/atari2600-games
    git -C work/atari2600-games checkout 0e16f397027ebb5d3be204c90efff169419ba5f8
    scripts/stage-homebrew-rom.sh work/atari2600-games flappy_the_duck

This copies `flappy_the_duck.bin` to ignored `work/test-roms/flappy_the_duck.a26` and prints its SHA-256. Read the matching `.txt` before use. The repository README says redistribution is approved only on the RetroBrew site/project; individual game rights vary. Do not commit or redistribute these ROMs without the rights holder's permission. See `docs/provenance/atari2600-homebrew-roms.md`.

## 7. Recovery notes

- Regenerate ignored `source/atari2600/prototype.gba` with `bash scripts/build-plugin.sh`.
- Regenerate `work/a26-sd.img` and related artifacts with `bash scripts/verify-harness.sh`.
- If CMake cannot find mGBA, restore the ignored clone at the pinned commit before changing CMake.
- `scripts/build-plugin.sh`, `scripts/build-fixture.sh`, and `scripts/build-card-fixture.py` call host devkitARM directly; none require Docker.
- `HANDOFF.md` has the short checkpoint. `PROGRESS.md` has current harness status, evidence, test results, and next task.
