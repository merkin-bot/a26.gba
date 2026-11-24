/* SPDX-License-Identifier: GPL-3.0-or-later */
.syntax unified
.cpu arm7tdmi
.section .rodata.atari_diagnostic_rom,"a",%progbits
.align 2
.global atari_diagnostic_rom
atari_diagnostic_rom:
  .incbin "../../test-roms/flappy.a26"
