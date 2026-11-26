#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
extra_sources=""
extra_flags=""
output=prototype
opt_level="${A26_OPT_LEVEL:-3}"
case "$opt_level" in
  0|1|2|3|s) ;;
  *) echo "A26_OPT_LEVEL must be 0, 1, 2, 3, or s" >&2; exit 2 ;;
esac
lto="${A26_LTO:-0}"
case "$lto" in
  0) lto_flags="" ;;
  1) lto_flags="-flto" ;;
  *) echo "A26_LTO must be 0 or 1" >&2; exit 2 ;;
esac
if [[ "${A26_FIXTURE:-0}" == "1" ]]; then
  extra_sources=" fixture.S"
  extra_flags="-DA26_FIXTURE"
  output=fixture
fi
case "${A26_RENDER_MODE:-mode3}" in
  mode3) ;;
  mode4) extra_flags+=" -DA26_MODE4_RENDER" ;;
  *) echo "A26_RENDER_MODE must be mode3 or mode4" >&2; exit 2 ;;
esac
"${DEVKITARM:-/opt/devkitpro/devkitARM}/bin/arm-none-eabi-gcc" -std=gnu89 -O"$opt_level" $lto_flags -mthumb -mthumb-interwork -mcpu=arm7tdmi -ffunction-sections -fdata-sections -fwrapv -fno-strict-aliasing -fcommon -Wno-implicit-int -Wno-implicit-function-declaration -DA26_GBA_BUILD=1 $extra_flags -I. -specs=gba.specs -nostartfiles startup.S main.c collisio.c cpu.c display.c files.c keyboard.c misc.c mouse.c raster.c table.c vmachine.c memory.c ui.c timer.c tia_audio.c pogo_harness_layout.c goomba_font.S $extra_sources -Wl,--gc-sections,-Map=$output.map -o $output.elf
"${DEVKITARM:-/opt/devkitpro/devkitARM}/bin/arm-none-eabi-objcopy" -O binary $output.elf $output.gba
"${DEVKITPRO:-/opt/devkitpro}/tools/bin/gbafix" $output.gba
if [[ "$output" == "prototype" ]]; then
  mkdir -p ../../candidate
  cp prototype.gba ../../candidate/a26.gba
fi
