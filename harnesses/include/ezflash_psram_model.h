#ifndef EZFLASH_PSRAM_MODEL_H
#define EZFLASH_PSRAM_MODEL_H

#include <stdint.h>

#define EZFLASH_PSRAM_BASE 0x08800000u
#define EZFLASH_PSRAM_WINDOW_BYTES 0x00800000u
#define EZFLASH_PSRAM_CAPACITY_BYTES 0x02000000u

typedef struct {
    uint8_t *bytes;
    uint16_t page;
    uint16_t rom_page;
    uint8_t unlock_stage;
    uint8_t page_committed;
    uint64_t page_writes;
    uint64_t rom_page_writes;
    uint64_t reads;
    uint64_t writes;
    uint64_t out_of_range;
} EZFlashPSRAMModel;

int ezflash_psram_model_init(EZFlashPSRAMModel *model);
void ezflash_psram_model_deinit(EZFlashPSRAMModel *model);
int ezflash_psram_model_read(EZFlashPSRAMModel *model, uint32_t address,
                             unsigned width, uint32_t *value);
int ezflash_psram_model_write(EZFlashPSRAMModel *model, uint32_t address,
                              unsigned width, uint32_t value);

#endif
