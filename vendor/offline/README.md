# Offline harness inputs

This directory contains the pinned source/toolchain inputs and an Ubuntu 24.04 x86_64 offline package repository used to rebuild and run the GBA/card harnesses and desktop Stella captures without contacting upstream services.

- `sources/mgba-431935616c0e216d650b25b2ea0c0acccd359038.tar.gz`: source snapshot of mGBA, MPL-2.0.
- `sources/dssmm-37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4.bundle`: local Git bundle with DSSMM history and EZ release tags / SuperFW Build769 source, Apache-2.0.
- `sources/pocketnes-9c07f3d1d2a26b957769c5cde942060cc9ea6e64.bundle`: buildable PocketNES source control. The upstream repository has no top-level license file; the repository owner authorizes GitHub source use under GPL-2.0. Preserve this provenance; do not publish built PocketNES binaries from this project.
- `sources/libgba-b8fa7f9c5c47289ec2e1e199d970042315bc1900.bundle`: devkitPro libgba v0.5.4 source, including its LGPL license and static-linking exception. The cold-start script builds and installs it into the ignored local devkitPro tree because DSSMM kernels link `-lgba` and `-lmm`.
- `sources/maxmod-ce8adb2a6bf1f29ec203b6c3410c1ddf8e270811.bundle`: devkitPro Maxmod GBA source at packaged v1.0.15 commit, including its license. The cold-start script builds `libmm.a` locally; DSSMM links it as `-lmm`.
- `toolchain/devkitpro-*.part`: chunks of the immutable official `devkitpro/devkitarm:20260610` archive, digest `sha256:116afba8df8453961de2936ffab20dd441edf4d682856c1ec8b0e53d7ed0bbf5`; includes devkitARM GCC 16.1.0, `gbafix`, and devkitPro tools.
- `packages/*.deb`: Ubuntu 24.04 amd64 packages for the host build tools, Python Unicorn/lcov DSSMM checks, FAT image tools, stock mGBA, and Stella/Xvfb/xdotool/Pillow capture path. Individual package copyright/license notices are retained inside each Debian package. `Packages` and `Packages.gz` form a local apt repository. `scripts/install-offline-host-tools.sh` points apt only at this local repository and passes `--no-download` for installation.

`SHA256SUMS` covers all source archives, package files, package indices, and toolchain chunks. `scripts/setup-offline-workspace.py` verifies it, restores ignored source checkouts and toolchain under `work/`, builds libgba and Maxmod locally, and makes no network calls.

The supported cold-start platform is a bootable x86_64 Ubuntu 24.04 OS with a shell, apt/dpkg, sudo, and libc/kernel already present. Those are the unavoidable platform base. After checkout, run `scripts/install-offline-host-tools.sh` only if needed, then `scripts/offline-baseline.sh`. No package, source, toolchain, or emulator is pulled from outside this repository by that path. The package set is pinned to its recorded Ubuntu archive versions; refreshing it requires updating the hashes and rerunning the offline clean-start qualification.

No commercial game ROMs, BIOS files, proprietary card firmware, generated kernels, SD images, or test ROMs with restricted redistribution rights are included.
