# GBA, DSSMM card, and SD harness

The harness has three independent layers:

1. **GBA system:** mGBA executes the kernel/plugin GBA code and models GBA CPU, memory, timers, video, DMA, and input.
2. **Flash card and kernel:** the matching DSSMM kernel runs and interacts with a per-model card profile. Do not merge different card mappings or kernel builds into one generic profile.
3. **SD storage:** each card profile reads a block image containing that card's matching kernel/support files, the A26 plugin, and the open-source Atari ROM. FAT/filesystem operations must cross the emulated card's SD interface; a host FAT image check alone does not prove this path.

The six target profiles and exact DSSMM kernel artifacts are listed in `cards.json`. Kernels are maintained in [voluptuary/dssmm](https://github.com/voluptuary/dssmm); local audit snapshot: `f6350776f07eba91ba2ffa34c6833d85f348f2a1`.

## Reuse existing DSSMM tests

DSSMM already has native mGBA launch fixtures for all three EZ-FLASH kernel lanes. Run from `harnesses/third_party/dssmm/`:

```sh
bash ezfomegade/scripts/test-native-launch-mgba.sh
bash ezfomega/scripts/test-native-launch-mgba.sh
bash ezfblue/scripts/test-native-launch-mgba.sh
```

These execute real launch UI/path/metadata code, but mock save, FAT transfer, patch, and card boot/page side effects. They are useful kernel regressions, not complete card or SD emulation. SuperFW has native menu/renderer tests and documents that they do not initialize physical SD, NOR, or cartridge SDRAM interfaces.

## Build context

DSSMM fixtures need devkitARM. In this Codespace the verified build path is the `devkitpro/devkitarm:latest` Docker image, while mGBA/Xvfb run on the host. Absolute workspace paths matter because fixtures generate absolute include paths. A previous `build-launch/` created under `/work` can leave stale `.d` dependencies; preserve/rename those generated directories before rebuilding under `/workspaces/a26.gba`.

The local mGBA/DSSMM source checkouts are ignored by Git. Do not commit mGBA, DSSMM kernel binaries, BIOS images, card firmware, or generated SD images.
