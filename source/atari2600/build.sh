#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
extra_sources=""
extra_flags=""
output=prototype
if [[ "${A26_FIXTURE:-0}" == "1" ]]; then
  extra_sources=" fixture.S"
  extra_flags="-DA26_FIXTURE"
  output=fixture
fi
"${DEVKITARM:-/opt/devkitpro/devkitARM}/bin/arm-none-eabi-gcc" -std=gnu89 -O2 -mthumb -mthumb-interwork -mcpu=arm7tdmi -ffunction-sections -fdata-sections -fwrapv -fno-strict-aliasing -fcommon -Wno-implicit-int -Wno-implicit-function-declaration -DA26_GBA_BUILD=1 $extra_flags -I. -specs=gba.specs -nostartfiles startup.S main.c collisio.c cpu.c display.c files.c keyboard.c misc.c mouse.c raster.c table.c vmachine.c memory.c ui.c timer.c tia_audio.c goomba_font.S $extra_sources -Wl,--gc-sections,-Map=$output.map -o $output.elf
"${DEVKITARM:-/opt/devkitpro/devkitARM}/bin/arm-none-eabi-objcopy" -O binary $output.elf $output.gba
"${DEVKITPRO:-/opt/devkitpro}/tools/bin/gbafix" $output.gba
