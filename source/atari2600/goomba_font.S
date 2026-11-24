/* SPDX-License-Identifier: GPL-2.0-only */
/* Goomba/PocketNES font asset; permission granted for use. */
.section .rodata.goomba_font,"a",%progbits
.align 2
.global goomba_font
goomba_font:
.incbin "font/goomba-font.lz77"
