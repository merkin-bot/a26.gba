# Atari 2600 homebrew test ROM catalog

The repository [`retrobrews/atari2600-games`](https://github.com/retrobrews/atari2600-games) is a useful catalog of additional Atari 2600 homebrew titles. It was inspected at commit `0e16f397027ebb5d3be204c90efff169419ba5f8`; its files include game `.bin` images and per-game `.txt` metadata.

## Rights and test handling

Do not assume the entire catalog is open source or redistributable. Its README says the ROMs were approved for free distribution on the RetroBrew site/project only and asks users to contact the owner/developer to share elsewhere. Individual `.txt` files can include additional restrictions. Review the relevant metadata and rights before using a game beyond local testing. Do not add catalog ROMs to this repository, commit them, or redistribute them without permission from the relevant rights holder.

To stage one locally for the emulator's `.a26` extension, clone the repository under ignored `work/` and copy a selected `.bin` under its `.a26` name:

    git clone https://github.com/retrobrews/atari2600-games work/atari2600-games
    git -C work/atari2600-games checkout 0e16f397027ebb5d3be204c90efff169419ba5f8
    scripts/stage-homebrew-rom.sh work/atari2600-games flappy_the_duck

The helper requires the matching `.txt` file, copies rather than edits the clone, prints the staged SHA-256, and writes to ignored `work/test-roms/`. For example, `flappy_the_duck.bin` stages as `work/test-roms/flappy_the_duck.a26`. Use other stems present in that checkout only after reading each game's metadata. These local assets must remain out of Git.
