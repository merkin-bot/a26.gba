#include "supercard_model.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define SC_MODE_REG 0x09FFFFFEu
#define SC_RDWR_COMMAND 0x09800000u
#define SC_WRITE_REGISTER_8 0x09000000u
#define SC_WRITE_REGISTER_A 0x0B000000u
#define SC_READ_REGISTER_8 0x09100000u
#define SC_READ_REGISTER_A 0x0B100000u
#define SC_MAGIC 0xA55Au

static int load_sd_data_block(SuperCardModel *model) {
    memset(model->sd_data_buffer, 0, sizeof(model->sd_data_buffer));
    model->sd_data_offset = 0;
    int ok = supercard_sd_read(model, model->sd_data_lba,
                               model->sd_data_buffer, 1);
    if (ok) {
        ++model->sd_data_blocks_loaded;
    } else {
        ++model->sd_data_load_failures;
    }
    return ok;
}

void supercard_reset(SuperCardModel *model) {
    model->mode = SUPERCARD_MODE_FIRMWARE;
    model->sd_interface = 0;
    model->write_access = 0;
    model->mode_magic_writes = 0;
    model->mode_value_writes = 0;
    model->pending_mode = 0;
    model->sd_command_write_count = 0;
    model->sd_response_length = 0;
    model->sd_response_bit = 0;
    model->sd_application_command = 0;
    model->sd_data_active = 0;
    model->sd_data_token_pending = 0;
    model->sd_data_token_delay = 0;
    model->sd_data_lba = 0;
    model->sd_data_offset = 0;
}

int supercard_write(SuperCardModel *model, uint32_t address, uint16_t value) {
    if (address != SC_MODE_REG)
        return 0;
    if (model->mode_magic_writes < 2 && value == SC_MAGIC) {
        ++model->mode_magic_writes;
        model->mode_value_writes = 0;
        return 1;
    }
    if (model->mode_magic_writes == 2) {
        if (model->mode_value_writes == 0) {
            model->pending_mode = value;
            model->mode_value_writes = 1;
            return 1;
        }
        if (model->mode_value_writes == 1 && value == model->pending_mode) {
            model->mode = (value & 1u) ? SUPERCARD_MODE_SDRAM : SUPERCARD_MODE_FIRMWARE;
            model->sd_interface = (value & 2u) != 0;
            model->write_access = (value & 4u) != 0;
        }
    }
    model->mode_magic_writes = value == SC_MAGIC ? 1u : 0u;
    model->mode_value_writes = 0;
    model->pending_mode = 0;
    return 1;
}

int supercard_read(SuperCardModel *model, uint32_t address, uint32_t width,
                   uint32_t *value) {
    if (!model || !value || !model->sd_interface) {
        return 0;
    }
    if (address >= 0x09200000u && address <= 0x0920001Cu && width == 32) {
        if (!model->sd_data_active || model->sd_data_offset + 4u > 520u) {
            return 0;
        }
        *value = (uint32_t)model->sd_data_buffer[model->sd_data_offset] |
                 ((uint32_t)model->sd_data_buffer[model->sd_data_offset + 1u] << 8) |
                 ((uint32_t)model->sd_data_buffer[model->sd_data_offset + 2u] << 16) |
                 ((uint32_t)model->sd_data_buffer[model->sd_data_offset + 3u] << 24);
        model->sd_data_offset += 4u;
        if (model->sd_data_offset == 520u) {
            ++model->sd_data_lba;
            model->sd_data_token_pending = 1;
            model->sd_data_token_delay = 1;
            if (!load_sd_data_block(model)) {
                model->sd_data_active = 0;
                model->sd_data_token_pending = 0;
            }
        }
        return 1;
    }
    if (width != 16) {
        return 0;
    }
    /* DSSMM supercard_io.S polls CMD bit0 and DAT0 bit8 for idle-high. */
    if (address == SC_RDWR_COMMAND) {
        if (model->sd_response_length) {
            if (model->sd_response_bit == 0) {
                *value = 0;
            } else if (model->sd_response_bit <= model->sd_response_length) {
                *value = model->sd_response[model->sd_response_bit - 1u];
            } else {
                *value = 1;
            }
            ++model->sd_response_bits_read;
            if (++model->sd_response_bit > model->sd_response_length + 1u) {
                model->sd_response_length = 0;
                model->sd_response_bit = 0;
            }
        } else {
            *value = 0x0001u;
        }
        return 1;
    }
    if (address == SC_READ_REGISTER_8 && model->sd_data_active) {
        ++model->sd_data_token_polls;
        if (model->sd_data_token_pending) {
            if (model->sd_data_token_delay) {
                --model->sd_data_token_delay;
                *value = 0x0100u;
            } else {
                model->sd_data_token_pending = 0;
                *value = 0;
            }
        } else {
            *value = 0x0100u;
        }
        return 1;
    }
    if (address == SC_WRITE_REGISTER_8 || address == SC_WRITE_REGISTER_A ||
        address == SC_READ_REGISTER_8 || address == SC_READ_REGISTER_A) {
        *value = 0x0100u;
        return 1;
    }
    return 0;
}

static void queue_short_response(SuperCardModel *model, unsigned index,
                                 uint32_t payload) {
    size_t out = 0;
    model->sd_response[out++] = 0;
    for (int bit = 5; bit >= 0; --bit) {
        model->sd_response[out++] = (uint8_t)((index >> bit) & 1u);
    }
    for (int bit = 31; bit >= 0; --bit) {
        model->sd_response[out++] = (uint8_t)((payload >> bit) & 1u);
    }
    for (unsigned bit = 0; bit < 7; ++bit) {
        model->sd_response[out++] = 0;
    }
    model->sd_response[out++] = 1;
    model->sd_response_length = out;
    model->sd_response_bit = 0;
    ++model->sd_responses_issued;
}

static void queue_long_response(SuperCardModel *model,
                                const uint8_t payload[15]) {
    size_t out = 0;
    for (unsigned bit = 0; bit < 7; ++bit) {
        model->sd_response[out++] = 1;
    }
    for (size_t byte = 0; byte < 15; ++byte) {
        for (int bit = 7; bit >= 0; --bit) {
            model->sd_response[out++] = (uint8_t)((payload[byte] >> bit) & 1u);
        }
    }
    for (unsigned bit = 0; bit < 7; ++bit) {
        model->sd_response[out++] = 0;
    }
    model->sd_response[out++] = 1;
    model->sd_response_length = out;
    model->sd_response_bit = 0;
    ++model->sd_responses_issued;
}

int supercard_sd_command_write(SuperCardModel *model, uint32_t address,
                               uint32_t width, uint32_t value) {
    if (!model || !model->sd_interface || width != 16 ||
        address != SC_RDWR_COMMAND) {
        return 0;
    }
    if (model->sd_command_write_count >= 48u) {
        model->sd_command_write_count = 0;
    }
    model->sd_command_writes[model->sd_command_write_count++] = (uint16_t)value;
    if (model->sd_command_write_count == 48u) {
        uint8_t command[6];
        if (!supercard_sd_decode_command(model->sd_command_writes, 48u, command)) {
            model->sd_command_write_count = 0;
            return 1;
        }
        model->sd_command_write_count = 0;
        unsigned index = command[0] & 0x3Fu;
        uint32_t argument = ((uint32_t)command[1] << 24) |
                            ((uint32_t)command[2] << 16) |
                            ((uint32_t)command[3] << 8) | command[4];
        model->sd_response_length = 0;
        model->sd_response_bit = 0;
        model->sd_last_command = (uint8_t)index;
        model->sd_last_argument = argument;
        ++model->sd_commands_decoded;
        if (index == 0u) {
            model->sd_application_command = 0;
        } else if (index == 8u && argument == 0x000001AAu) {
            queue_short_response(model, index, argument);
        } else if (index == 55u) {
            model->sd_application_command = 1;
            queue_short_response(model, index, 0);
        } else if (index == 41u && model->sd_application_command) {
            model->sd_application_command = 0;
            queue_short_response(model, index, 0xC0040000u);
        } else if (index == 2u) {
            const uint8_t cid[15] = {0x03, 0x41, 0x32, 0x36, 0x47, 0x42,
                                     0x41, 0x10, 0x00, 0x00, 0x01, 0x12,
                                     0x34, 0x56, 0x78};
            queue_long_response(model, cid);
        } else if (index == 3u) {
            queue_short_response(model, index, 0x12340800u);
        } else if (index == 9u) {
            const uint8_t csd[15] = {0x40, 0x0E, 0x00, 0x32, 0x5B,
                                     0x59, 0x00, 0x00, 0x00, 0x7F,
                                     0x00, 0x80, 0x0A, 0x40, 0x01};
            queue_long_response(model, csd);
        } else if (index == 18u) {
            ++model->sd_read_commands;
            model->sd_data_lba = argument;
            model->sd_data_active = load_sd_data_block(model);
            model->sd_data_token_pending = model->sd_data_active;
            model->sd_data_token_delay = 0;
            queue_short_response(model, index, 0);
        } else if (index == 12u) {
            ++model->sd_stop_commands;
            model->sd_data_active = 0;
            model->sd_data_token_pending = 0;
            queue_short_response(model, index, 0);
        } else if (index == 6u || index == 7u || index == 16u) {
            queue_short_response(model, index, 0);
        } else {
            model->sd_application_command = 0;
        }
    }
    return 1;
}

int supercard_sd_decode_command(const uint16_t *writes, size_t write_count,
                               uint8_t command[6]) {
    if (!writes || !command || write_count != 48) {
        return 0;
    }
    memset(command, 0, 6);
    for (size_t bit = 0; bit < write_count; ++bit) {
        command[bit / 8] = (uint8_t)((command[bit / 8] << 1) |
                                    ((writes[bit] >> 7) & 1u));
    }
    return (command[0] & 0xC0u) == 0x40u;
}

void supercard_set_payload(SuperCardModel *model, uint32_t offset,
                           const uint8_t *data, size_t size) {
    if (!model->sdram || (!data && size) || model->mode != SUPERCARD_MODE_SDRAM ||
        !model->write_access || offset > model->sdram_size)
        return;
    size_t available = model->sdram_size - offset;
    if (size > available)
        size = available;
    memcpy(model->sdram + offset, data, size);
}

int supercard_sd_read(SuperCardModel *model, uint32_t lba,
                      uint8_t *buffer, size_t blocks) {
    if (!model->sd_path || !buffer || !model->sd_interface || blocks > SIZE_MAX / 512u)
        return 0;
    FILE *file = fopen(model->sd_path, "rb");
    if (!file)
        return 0;
    size_t bytes = blocks * 512u;
    uint64_t offset = (uint64_t)lba * 512u;
    if (offset > LONG_MAX || bytes > (uint64_t)LONG_MAX - offset ||
        fseek(file, (long)offset, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    size_t count = fread(buffer, 1, bytes, file);
    fclose(file);
    return count == bytes;
}

int supercard_sd_write(SuperCardModel *model, uint32_t lba,
                       const uint8_t *buffer, size_t blocks) {
    if (!model->sd_path || !buffer || !model->sd_interface || !model->write_access ||
        blocks > SIZE_MAX / 512u)
        return 0;
    FILE *file = fopen(model->sd_path, "r+b");
    if (!file)
        return 0;
    size_t bytes = blocks * 512u;
    uint64_t offset = (uint64_t)lba * 512u;
    if (offset > LONG_MAX || bytes > (uint64_t)LONG_MAX - offset ||
        fseek(file, (long)offset, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    size_t count = fwrite(buffer, 1, bytes, file);
    fclose(file);
    return count == bytes;
}
