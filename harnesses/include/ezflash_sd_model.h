#ifndef EZFLASH_SD_MODEL_H
#define EZFLASH_SD_MODEL_H

#include <stdint.h>
#include <stdio.h>

/* Synthetic interpretation of the GBA-side DSSMM EZ SD register transcript.
 * This is not a physical card or timing model. */
typedef struct {
    FILE *image;
    uint64_t image_size_bytes;
    uint32_t sector;
    uint16_t count_word;
    uint16_t status_value; /* synthetic; default ready value is zero */
    uint8_t control;
    uint8_t command_committed;
    uint8_t unlock_stage;
    uint8_t status_poll_pending;
    uint8_t sector_registers_dirty;
    uint8_t command_active;
    uint8_t write_command;
    uint8_t failed;
    uint8_t buffer[2048]; /* Source transfers are chunked to at most four sectors. */
    uint16_t buffer_bytes;
    uint16_t buffer_offset;
    uint64_t commands;
    uint64_t blocks_read;
    uint64_t blocks_written;
    uint64_t failures;
    uint32_t last_command_sector;
    uint16_t last_command_blocks;
    uint8_t last_command_write;
} EZFlashSDModel;

int ezflash_sd_model_open(EZFlashSDModel *model, const char *image_path,
                          uint16_t synthetic_ready_value);
void ezflash_sd_model_close(EZFlashSDModel *model);
int ezflash_sd_model_read(EZFlashSDModel *model, uint32_t address,
                          unsigned width, uint32_t *value);
int ezflash_sd_model_write(EZFlashSDModel *model, uint32_t address,
                           unsigned width, uint32_t value);

#endif
