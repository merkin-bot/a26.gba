#ifndef EZFLASH_NOR_MODEL_H
#define EZFLASH_NOR_MODEL_H

#include <stdint.h>

#define EZFLASH_S71_NOR_BASE 0x08000000u
#define EZFLASH_S98_NOR_BASE 0x09000000u
#define EZFLASH_S71_NOR_CAPACITY 0x00800000u

typedef struct {
    uint8_t *bytes;
    uint32_t base;
    uint32_t sector_bytes;
    uint16_t device_id;
    uint8_t command_stage;
    uint8_t id_mode;
    uint64_t erase_commands;
    uint64_t program_words;
    uint64_t reads;
    uint64_t writes;
    uint64_t invalid_commands;
} EZFlashNORModel;

int ezflash_nor_model_init(EZFlashNORModel *model, const char *kernel_path);
int ezflash_nor_model_init_window(EZFlashNORModel *model, uint32_t base,
                                  uint32_t sector_bytes,
                                  const char *initial_contents,
                                  uint16_t device_id);
void ezflash_nor_model_deinit(EZFlashNORModel *model);
int ezflash_nor_model_read(EZFlashNORModel *model, uint32_t address,
                           unsigned width, uint32_t *value);
int ezflash_nor_model_write(EZFlashNORModel *model, uint32_t address,
                            unsigned width, uint32_t value);

#endif
