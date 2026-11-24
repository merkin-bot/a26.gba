#include "pogo_harness_layout.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

int main(void) {
    assert(pogo_layout_file_offset(257, 8193) == 260);
    assert(pogo_layout_table_offset(257, 8193) == 32768);
    assert(pogo_layout_file_offset(0, 1) == 0);
    assert(pogo_layout_file_offset(14u*1024u*1024u, 1) == 0);

    uint32_t ram[24];
    pogo_seed_argument_block(ram, "/SYSTEM/PLUG/a26.gba",
                             "/ROMS/flappy.a26", 257, 4096);
    assert(ram[0] == 4096);
    assert(ram[1] == 0x08000104u);
    assert(ram[2] == 0xfab0babeu);
    assert(ram[3] == 2);
    puts("PogoShell layout/ABI smoke tests passed");
    return 0;
}
