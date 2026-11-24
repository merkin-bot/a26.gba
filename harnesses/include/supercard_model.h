#ifndef SUPERCARD_MODEL_H
#define SUPERCARD_MODEL_H
#include <stddef.h>
#include <stdint.h>

typedef enum {
    SUPERCARD_MODE_FIRMWARE = 0,
    SUPERCARD_MODE_SDRAM = 1
} SuperCardMapMode;

typedef struct {
    uint8_t *cart;
    size_t cart_size;
    uint8_t *sdram;
    size_t sdram_size;
    const char *sd_path;
    SuperCardMapMode mode;
    int sd_interface;
    int write_access;
    unsigned mode_magic_writes;
    unsigned mode_value_writes;
    uint16_t pending_mode;
    uint16_t sd_command_writes[48];
    size_t sd_command_write_count;
    uint8_t sd_response[136]; /* Native response bits after the start bit. */
    size_t sd_response_length;
    size_t sd_response_bit;
    uint64_t sd_commands_decoded;
    uint64_t sd_responses_issued;
    uint64_t sd_response_bits_read;
    uint8_t sd_last_command;
    int sd_application_command;
} SuperCardModel;

void supercard_reset(SuperCardModel *model);
int supercard_write(SuperCardModel *model, uint32_t address, uint16_t value);
int supercard_read(SuperCardModel *model, uint32_t address, uint32_t width,
                   uint32_t *value);
int supercard_sd_command_write(SuperCardModel *model, uint32_t address,
                               uint32_t width, uint32_t value);
int supercard_sd_decode_command(const uint16_t *writes, size_t write_count,
                               uint8_t command[6]);
void supercard_set_payload(SuperCardModel *model, uint32_t offset,
                           const uint8_t *data, size_t size);
int supercard_sd_read(SuperCardModel *model, uint32_t lba,
                      uint8_t *buffer, size_t blocks);
int supercard_sd_write(SuperCardModel *model, uint32_t lba,
                       const uint8_t *buffer, size_t blocks);
#endif
