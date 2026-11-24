#include "pogo_harness_layout.h"
#include <string.h>

#define ROM_BASE 0x08000000u
#define PAYLOAD_LIMIT (14u * 1024u * 1024u)

uint32_t pogo_layout_file_offset(uint32_t plugin_size, uint32_t rom_size) {
    if (!plugin_size || !rom_size || plugin_size > PAYLOAD_LIMIT || rom_size > PAYLOAD_LIMIT)
        return 0;
    uint32_t start = (plugin_size + 3u) & ~3u;
    if (rom_size > PAYLOAD_LIMIT - start)
        return 0;
    return start;
}

uint32_t pogo_layout_table_offset(uint32_t plugin_size, uint32_t rom_size) {
    uint32_t file_offset = pogo_layout_file_offset(plugin_size, rom_size);
    if (!file_offset)
        return 0;
    uint32_t table = (file_offset + rom_size + 32767u) & ~32767u;
    if (table > PAYLOAD_LIMIT - 88u)
        return 0;
    return table;
}

static void short_name(char out[32], const char *name) {
    const char *base = strrchr(name, '/');
    base = base ? base + 1 : name;
    size_t n = strlen(base);
    memset(out, 0, 32);
    if (n <= 31)
        memcpy(out, base, n);
    else
        memcpy(out, base, 31);
}

void pogo_seed_argument_block(uint32_t ram[24], const char *plugin,
                              const char *rom, uint32_t plugin_size,
                              uint32_t rom_size) {
    uint32_t file_offset = pogo_layout_file_offset(plugin_size, rom_size);
    memset(ram, 0, 96);
    ram[0] = rom_size;
    ram[1] = ROM_BASE + file_offset;
    ram[2] = 0xfab0babeu;
    ram[3] = 2;
    char *p = (char *)&ram[4];
    *p++ = '/';
    short_name((char *)p, plugin);
    p += strlen(p) + 1;
    *p++ = '/';
    short_name((char *)p, rom);
}
