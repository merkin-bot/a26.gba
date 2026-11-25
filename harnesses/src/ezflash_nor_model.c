#include "ezflash_nor_model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ezflash_nor_model_init_window(EZFlashNORModel *model, uint32_t base,
                                  uint32_t sector_bytes,
                                  const char *initial_contents,
                                  uint16_t device_id) {
    if (!model || !sector_bytes || (sector_bytes & (sector_bytes - 1u)) ||
        sector_bytes > EZFLASH_S71_NOR_CAPACITY) return 0;
    memset(model, 0, sizeof(*model));
    model->base = base;
    model->sector_bytes = sector_bytes;
    model->bytes = malloc(EZFLASH_S71_NOR_CAPACITY);
    if (!model->bytes) return 0;
    memset(model->bytes, 0xFF, EZFLASH_S71_NOR_CAPACITY);
    model->device_id = device_id;

    if (initial_contents) {
        FILE *file = fopen(initial_contents, "rb");
        if (!file) {
            ezflash_nor_model_deinit(model);
            return 0;
        }
        size_t copied = fread(model->bytes, 1, EZFLASH_S71_NOR_CAPACITY, file);
        int read_error = ferror(file);
        fclose(file);
        if (read_error || copied == 0) {
            ezflash_nor_model_deinit(model);
            return 0;
        }
    }
    return 1;
}

int ezflash_nor_model_init(EZFlashNORModel *model, const char *kernel_path) {
    if (!kernel_path) return 0;
    return ezflash_nor_model_init_window(model, EZFLASH_S71_NOR_BASE,
                                         0x10000u, kernel_path, 0x2202u);
}

void ezflash_nor_model_deinit(EZFlashNORModel *model) {
    if (!model) return;
    free(model->bytes);
    memset(model, 0, sizeof(*model));
}

static int nor_offset(EZFlashNORModel *model, uint32_t address,
                      unsigned width, uint32_t *offset) {
    if (!model || !model->bytes || !offset ||
        (width != 8 && width != 16 && width != 32) ||
        address < model->base ||
        address >= model->base + EZFLASH_S71_NOR_CAPACITY ||
        (address & ((width / 8u) - 1u))) return 0;
    *offset = address - model->base;
    return 1;
}

int ezflash_nor_model_read(EZFlashNORModel *model, uint32_t address,
                           unsigned width, uint32_t *value) {
    uint32_t offset;
    if (!value || !nor_offset(model, address, width, &offset)) return 0;
    if (model->id_mode) {
        uint16_t id = offset == 0x1Cu ? model->device_id :
                      offset == 0 ? 0x0022u : 0xFFFFu;
        *value = id;
        if (width == 32) *value |= (uint32_t)id << 16;
    } else {
        *value = model->bytes[offset];
        if (width >= 16) *value |= (uint32_t)model->bytes[offset + 1u] << 8;
        if (width == 32) {
            *value |= (uint32_t)model->bytes[offset + 2u] << 16;
            *value |= (uint32_t)model->bytes[offset + 3u] << 24;
        }
    }
    ++model->reads;
    return 1;
}

int ezflash_nor_model_write(EZFlashNORModel *model, uint32_t address,
                            unsigned width, uint32_t value) {
    if (!model || !model->bytes) return 0;
    if (width == 16 && address == model->base && value == 0x00F0u) {
        model->id_mode = 0;
        model->command_stage = 0;
        return 1;
    }
    if (width != 16 || address < model->base ||
        address >= model->base + EZFLASH_S71_NOR_CAPACITY) return 0;

    uint32_t unlock1 = model->base + 0x555u * 2u;
    uint32_t unlock2 = model->base + 0x2AAu * 2u;

    switch (model->command_stage) {
    case 0:
        if (address == unlock1 && value == 0x00AAu) {
            model->command_stage = 1;
            return 0;
        }
        break;
    case 1:
        model->command_stage = address == unlock2 && value == 0x0055u ? 2u : 0u;
        return 0;
    case 2:
        if (address == unlock1 && value == 0x0090u) {
            model->id_mode = 1;
            model->command_stage = 0;
            return 0;
        }
        if (address == unlock1 && value == 0x0080u) {
            model->command_stage = 3;
            return 0;
        }
        if (address == unlock1 && value == 0x00A0u) {
            model->command_stage = 5;
            return 0;
        }
        model->command_stage = 0;
        break;
    case 3:
        model->command_stage = address == unlock1 && value == 0x00AAu ? 4u : 0u;
        return 0;
    case 4:
        model->command_stage = address == unlock2 && value == 0x0055u ? 6u : 0u;
        return 0;
    case 5: {
        uint32_t offset;
        model->command_stage = 0;
        if (!nor_offset(model, address, 16, &offset)) break;
        model->bytes[offset] &= (uint8_t)value;
        model->bytes[offset + 1u] &= (uint8_t)(value >> 8);
        ++model->program_words;
        ++model->writes;
        return 1;
    }
    case 6:
        model->command_stage = 0;
        if (value == 0x0030u) {
            uint32_t offset = address - model->base;
            uint32_t sector = offset & ~(model->sector_bytes - 1u);
            memset(model->bytes + sector, 0xFF, model->sector_bytes);
            ++model->erase_commands;
            ++model->writes;
            return 1;
        }
        break;
    default:
        model->command_stage = 0;
        break;
    }

    ++model->invalid_commands;
    return 0;
}
