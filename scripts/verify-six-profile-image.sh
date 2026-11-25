#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

pocketnes="${POCKETNES_PLUGIN_GBA:-work/PocketNES/pocketnes.gba}"
snailmaze="${SNAILMAZE_ROM:-work/retrobrews-nes-games/snailmaze.nes}"
base_image="${SIX_PROFILE_BASE_IMAGE:-work/six-profile-exfat.img}"
run_image="${SIX_PROFILE_RUN_IMAGE:-work/six-profile-run.img}"
results="${SIX_PROFILE_RESULTS:-work/six-profile-results}"
image_mib="${SIX_PROFILE_IMAGE_MIB:-4096}"

for input in "$pocketnes" "$snailmaze"; do
  if [[ ! -s "$input" ]]; then
    echo "missing shared control input: $input" >&2
    exit 2
  fi
done
for runner in build/harness-cmake/ezflash-mgba-runner build/harness-cmake/supercard-mgba-runner; do
  if [[ ! -x "$runner" ]]; then
    echo "missing runner: $runner (build the documented harness targets first)" >&2
    exit 2
  fi
done

CONTROL_PLUGIN_GBA="$pocketnes" CONTROL_PLUGIN_EXT=nes \
  CONTROL_ROM="$snailmaze" CONTROL_ROM_NAME=snailmaze.nes \
  bash scripts/make-sd-image.sh "$base_image" "$image_mib" exfat
fsck.exfat -n "$base_image"
base_hash="$(sha256sum "$base_image" | awk '{print $1}')"
mkdir -p "$results"

run_profile() {
  local id="$1" board="$2" kernel="$3" kind="$4" events="$5"
  local log="$results/$id.log" capture="$results/$id.ppm" before after
  test -s "$kernel"
  cp "$base_image" "$run_image"
  before="$(sha256sum "$run_image" | awk '{print $1}')"
  test "$before" = "$base_hash"
  fsck.exfat -n "$run_image" >/dev/null
  if [[ "$kind" == ez ]]; then
    A26_EZ_TRACE_PC=1 build/harness-cmake/ezflash-mgba-runner \
      "$kernel" 3200 "$run_image" "$capture" "$events" >"$log"
    rg -q 'failures=0' "$log"
    rg -q 'psram_reads=[1-9][0-9]*([[:space:]]|$)' "$log"
  else
    A26_SUPERCARD_BOARD="$board" build/harness-cmake/supercard-mgba-runner \
      "$kernel" 3200 "$run_image" "$capture" "$events" >"$log"
    rg -q 'region=cartridge' "$log"
    rg -q 'load_failures=0' "$log"
  fi
  fsck.exfat -n "$run_image" >/dev/null
  after="$(sha256sum "$run_image" | awk '{print $1}')"
  printf 'profile=%s baseline_image_sha256=%s post_run_image_sha256=%s log=%s capture=%s\n' \
    "$id" "$before" "$after" "$log" "$capture"
}

ez_events='350-360:0x80,380-390:0x1,450-470:0x1,550-580:0x1'
sc_events='600:0x1,1200:0x1,1800:0x80,1900:0x1,2200-2210:0x1'
run_profile ezfomegade ezfomegade harnesses/third_party/dssmm/ezfomegade/ezfomegade.gba ez "$ez_events"
run_profile ezfomega ezfomega harnesses/third_party/dssmm/ezfomega/ezfomega.gba ez "$ez_events"
run_profile ezfblue ezfblue harnesses/third_party/dssmm/ezfblue/targets/blue/ezbluekernel-development.gba ez "$ez_events"
run_profile supercard-sd sd harnesses/third_party/dssmm/superfw/build/sd/superfw.fw supercard "$sc_events"
run_profile supercard-lite lite harnesses/third_party/dssmm/superfw/build/lite/superfw.fw supercard "$sc_events"
run_profile supercard-chis chis harnesses/third_party/dssmm/superfw/build/chis/superfw.fw supercard "$sc_events"

echo "PASS: the same pristine exFAT image hash was restored before all six matching kernel/model runs."
