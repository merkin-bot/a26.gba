#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

root="harnesses/third_party/dssmm"
repo_pin=37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4
restore_checkout() {
  git -C "$root" checkout "$repo_pin" >/dev/null 2>&1 || true
}
trap restore_checkout EXIT

if [[ -f /opt/devkitpro/devkitarm.sh ]]; then
  source /opt/devkitpro/devkitarm.sh
elif [[ -f /etc/profile.d/devkit-env.sh ]]; then
  source /etc/profile.d/devkit-env.sh
fi
: "${DEVKITARM:?devkitARM environment is not configured; follow WORKSPACE.md}"
bash scripts/build-plugin.sh
cmake -S harnesses -B build/harness-cmake -G Ninja
cmake --build build/harness-cmake --target ezflash-mgba-runner -j2

while IFS='|' read -r tag lane kernel_rel; do
  [[ -n "$tag" ]] || continue
  git -C "$root" checkout "$tag"
  bash "$root/$lane/scripts/test-native-launch-mgba.sh"
  make -C "$root/$lane" clean
  make -C "$root/$lane" -j2
  kernel="$root/$lane/$kernel_rel"
  image="work/ez-${lane}-sd.img"
  capture="work/ez-${lane}-300.ppm"
  test -s "$kernel"
  KERNEL_GBA="$kernel" bash scripts/make-sd-image.sh "$image" 64
  fsck.fat -n "$image" >/dev/null
  build/harness-cmake/ezflash-mgba-runner "$kernel" 300 "$image" "$capture"

  kernel_hash="$(sha256sum "$kernel" | awk '{print $1}')"
  image_kernel_hash="$(mtype -i "$image" ::SYSTEM/KERNELS/KERNEL.GBA | sha256sum | awk '{print $1}')"
  plugin_hash="$(mtype -i "$image" ::SYSTEM/PLUG/a26.gba | sha256sum | awk '{print $1}')"
  rom_hash="$(mtype -i "$image" ::ROMS/flappy.a26 | sha256sum | awk '{print $1}')"
  test "$kernel_hash" = "$image_kernel_hash"
  test "$plugin_hash" = "cc868252fb113cd4160b8c6677d39cfe076056a4422f262b0975c99f5fb21de7"
  test "$rom_hash" = "ad7bc56f0e4ae5938cc06f7cbd9be8a2cc8de715ca4a3eadfb057fd9d260afcb"
  echo "profile=$lane kernel_sha256=$kernel_hash image_sha256=$(sha256sum "$image" | awk '{print $1}') plugin_sha256=$plugin_hash rom_sha256=$rom_hash"
done <<'PROFILES'
v8.4.1|ezfomegade|ezfomegade.gba
v8.4.1-omega-test5|ezfomega|ezfomega.gba
v8.4.1-blue-test5|ezfblue|targets/blue/ezbluekernel-development.gba
PROFILES

restore_checkout
trap - EXIT
