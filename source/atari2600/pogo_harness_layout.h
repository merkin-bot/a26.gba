#ifndef A26_POGO_HARNESS_LAYOUT_H
#define A26_POGO_HARNESS_LAYOUT_H
#include <stdint.h>

uint32_t pogo_layout_file_offset(uint32_t plugin_size, uint32_t rom_size);
uint32_t pogo_layout_table_offset(uint32_t plugin_size, uint32_t rom_size);
void pogo_seed_argument_block(uint32_t ram[24], const char *plugin,
                              const char *rom, uint32_t plugin_size,
                              uint32_t rom_size);
#endif
