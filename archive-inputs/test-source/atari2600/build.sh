#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
"${DEVKITARM:-/opt/devkitpro/devkitARM}/bin/arm-none-eabi-gcc" -std=gnu89 -O2 -mthumb -mthumb-interwork -mcpu=arm7tdmi -ffunction-sections -fdata-sections -fwrapv -fno-strict-aliasing -fcommon -Wno-implicit-int -Wno-implicit-function-declaration -I. -specs=gba.specs -nostartfiles startup.S main.c collisio.c cpu.c display.c files.c keyboard.c misc.c mouse.c raster.c table.c vmachine.c memory.c ui.c timer.c -Wl,--gc-sections,-Map=prototype.map -o prototype.elf
"${DEVKITARM:-/opt/devkitpro/devkitARM}/bin/arm-none-eabi-objcopy" -O binary prototype.elf prototype.gba
"${DEVKITPRO:-/opt/devkitpro}/tools/bin/gbafix" prototype.gba
