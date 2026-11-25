# Build the workspace in a fresh Codespace

Use a regular Ubuntu 24.04 Codespace with the unrestricted host environment. This repository does not use a devcontainer or Docker. The previous Docker-in-Docker setup could not start inside the restricted Codespace (dockerd was denied iptables/NAT access), so `.devcontainer/` has been removed. Run these commands in the Codespace terminal; sudo is required for system packages and devkitPro.

## 1. Install host tools

    sudo apt-get update
    sudo apt-get install -y build-essential cmake ninja-build pkg-config curl git unzip zip python3 libepoxy-dev libgl1-mesa-dev libsdl2-dev libqt5opengl5-dev qtbase5-dev libpng-dev libzip-dev zlib1g-dev libavcodec-dev libavformat-dev libavutil-dev libswresample-dev libswscale-dev libedit-dev libelf-dev mgba-sdl xvfb xdotool xauth stella dosfstools mtools

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
    gbafix --help >/dev/null
    command -v cmake ninja cc python3 /usr/games/mgba xvfb-run xdotool stella mkfs.vfat mcopy fsck.fat

If the official bootstrap URL returns HTTP 403, autonomously follow the current installation alternatives linked from devkitPro's Linux instructions; do not restore Docker-in-Docker or substitute an unrelated ARM compiler. The prior restricted Codespace received HTTP 403, so confirm access in the new unrestricted Codespace. Historical builds used devkitARM GCC 16.1.0 from `devkitpro/devkitarm:latest`. The pacman package may be newer; record compiler version and plugin hash in `PROGRESS.md`. Claim exact archived hash reproduction only if the result matches.

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
    cmake --build build/harness-cmake --target supercard-model-test mgba-core-smoke supercard-mgba-runner ezflash-mgba-trace-smoke -j2
    build/harness-cmake/supercard-model-test
    build/harness-cmake/mgba-core-smoke
    build/harness-cmake/ezflash-mgba-trace-smoke

CMake may report the supercard-mgba-runner unavailable if `harnesses/src/mgba_supercard_runner.c` is absent; remaining targets still configure and build. The baseline builds the plugin, checks the PogoShell ABI, creates the virtual FAT image, builds a standalone fixture, and runs it under stock mGBA/Xvfb. Some checks repeat; the baseline is the broad initial verification.

For SuperCard diagnostics, use the matching DSSMM kernel from the pinned source and follow `harnesses/README.md` plus `PROGRESS.md`. Run `make clean` before switching SuperFW `BOARD` among `chis`, `sd`, and `lite` because generated intermediate names overlap. Mark card behavior `UNVERIFIED` where evidence is absent.

## 5. Recovery notes

- Regenerate ignored `source/atari2600/prototype.gba` with `bash scripts/build-plugin.sh`.
- Regenerate `work/a26-sd.img` and related artifacts with `bash scripts/verify-harness.sh`.
- If CMake cannot find mGBA, restore the ignored clone at the pinned commit before changing CMake.
- `scripts/build-plugin.sh`, `scripts/build-fixture.sh`, and `scripts/build-card-fixture.py` call host devkitARM directly; none require Docker.
- `HANDOFF.md` has the short checkpoint. `PROGRESS.md` has current harness status, evidence, test results, and next task.
