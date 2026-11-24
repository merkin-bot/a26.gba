# Card Harness

The host SuperCard model is buildable without mGBA. The mGBA runner uses the
same CMake shape as the Apache-2.0 EZ-FLASH Air reference harness.

Build:

```sh
git clone --depth 1 https://github.com/mgba-emu/mgba harnesses/third_party/mgba
cmake -S harnesses -B build/harness-cmake -G Ninja
ninja -C build/harness-cmake supercard-model-test supercard-mgba-runner
build/harness-cmake/supercard-model-test
```
