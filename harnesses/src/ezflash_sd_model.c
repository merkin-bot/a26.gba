#include "ezflash_sd_model.h"

#include <string.h>

#define EZ_COMMAND_END 0x09FC0000u
#define EZ_COMMAND_UNLOCK 0x09FE0000u
#define EZ_CONTROL 0x09400000u
#define EZ_STATUS_DATA 0x09E00000u
#define EZ_SECTOR_LOW 0x09600000u
#define EZ_SECTOR_HIGH 0x09620000u
#define EZ_COUNT 0x09640000u

static int advance_block(EZFlashSDModel *model);

static int start_command(EZFlashSDModel *model) {
    uint32_t count = model->count_word & 0x7FFFu;
    model->write_command = (model->count_word & 0x8000u) != 0;
    if (!model->write_command) model->buffer_offset = 0;
    model->buffer_bytes = (uint16_t)(count <= 4 ? count * 512u : 0);
    model->failed = 0;
    model->command_active = count != 0 && count <= 4 && model->control != 0;
    if (model->sector > UINT64_MAX / 512u ||
        (uint64_t)model->sector * 512u + model->buffer_bytes > model->image_size_bytes)
        model->command_active = 0;
    ++model->commands;
    if (!model->command_active) {
        model->failed = 1;
        ++model->failures;
    }
    if (model->command_active && model->write_command &&
        model->buffer_offset == model->buffer_bytes) {
        if (fseek(model->image, (long)((uint64_t)model->sector * 512u), SEEK_SET) ||
            fwrite(model->buffer, 1, model->buffer_bytes, model->image) != model->buffer_bytes ||
            fflush(model->image)) {
            model->failed = 1;
            model->command_active = 0;
            ++model->failures;
        } else {
            (void)advance_block(model);
        }
    }
    return model->command_active;
}

static int advance_block(EZFlashSDModel *model) {
    if (!model->command_active) return 0;
    uint32_t count = model->count_word & 0x7FFFu;
    if (model->buffer_offset < model->buffer_bytes) return 1;
    uint32_t transferred = model->buffer_bytes / 512u;
    model->sector += transferred;
    if (model->write_command) model->blocks_written += transferred;
    else model->blocks_read += transferred;
    model->command_active = 0;
    return model->command_active;
}

int ezflash_sd_model_open(EZFlashSDModel *model, const char *image_path,
                          uint16_t synthetic_ready_value) {
    if (!model || !image_path || synthetic_ready_value == 0xEEE1u) return 0;
    memset(model, 0, sizeof(*model));
    model->image = fopen(image_path, "r+b");
    if (!model->image) model->image = fopen(image_path, "rb");
    if (!model->image) return 0;
    if (fseek(model->image, 0, SEEK_END) != 0) {
        ezflash_sd_model_close(model);
        return 0;
    }
    long image_size = ftell(model->image);
    if (image_size < 0 || fseek(model->image, 0, SEEK_SET) != 0) {
        ezflash_sd_model_close(model);
        return 0;
    }
    model->image_size_bytes = (uint64_t)image_size;
    model->status_value = synthetic_ready_value;
    return 1;
}

void ezflash_sd_model_close(EZFlashSDModel *model) {
    if (model && model->image) fclose(model->image);
    if (model) memset(model, 0, sizeof(*model));
}

int ezflash_sd_model_read(EZFlashSDModel *model, uint32_t address,
                          unsigned width, uint32_t *value) {
    if (!model || !model->image || !value || (width != 16 && width != 32))
        return 0;
    if (address == EZ_STATUS_DATA && model->status_poll_pending) {
        if (width != 16) return 0;
        *value = model->status_value;
        model->status_poll_pending = 0;
        return 1;
    }
    if (address < EZ_STATUS_DATA || address >= EZ_STATUS_DATA + 2048u ||
        (address & 1u)) return 0;
    if (!model->command_active || model->write_command) {
        *value = 0xFFFFu;
        return 1;
    }
    if (model->buffer_offset == 0) {
        if (fseek(model->image, (long)((uint64_t)model->sector * 512u), SEEK_SET) ||
            fread(model->buffer, 1, model->buffer_bytes, model->image) != model->buffer_bytes) {
            model->failed = 1;
            model->command_active = 0;
            ++model->failures;
            *value = 0xFFFFu;
            return 1;
        }
    }
    uint16_t offset = (uint16_t)(address - EZ_STATUS_DATA);
    unsigned bytes = width / 8u;
    if (offset + bytes > model->buffer_bytes) return 0;
    *value = (uint32_t)model->buffer[offset] |
             ((uint32_t)model->buffer[offset + 1u] << 8);
    if (width == 32) *value |= (uint32_t)model->buffer[offset + 2u] << 16 |
                                (uint32_t)model->buffer[offset + 3u] << 24;
    model->buffer_offset = (uint16_t)(offset + bytes);
    if (model->buffer_offset == model->buffer_bytes) (void)advance_block(model);
    return 1;
}

int ezflash_sd_model_write(EZFlashSDModel *model, uint32_t address,
                           unsigned width, uint32_t value) {
    if (!model || !model->image || (width != 16 && width != 32)) return 0;
    if (address == EZ_COMMAND_UNLOCK) {
        model->unlock_stage = value == 0xD200u ? 1u : 0u;
        model->command_committed = 0;
        return 1;
    }
    if (address == 0x08000000u) {
        model->unlock_stage = model->unlock_stage == 1u && value == 0x1500u
            ? 2u : 0u;
        return 0; /* GBA cartridge access is still delegated by the adapter. */
    }
    if (address == 0x08020000u) {
        model->unlock_stage = model->unlock_stage == 2u && value == 0xD200u
            ? 3u : 0u;
        return 0;
    }
    if (address == 0x08040000u) {
        model->command_committed = model->unlock_stage == 3u &&
                                   value == 0x1500u;
        model->unlock_stage = model->command_committed ? 4u : 0u;
        return 0; /* Preserve upstream GBA cartridge callback behavior. */
    }
    if (address == EZ_SECTOR_LOW) {
        if (!model->command_committed) return 0;
        model->sector = (model->sector & 0xFFFF0000u) | (uint16_t)value;
        model->sector_registers_dirty = 1;
        return 1;
    }
    if (address == EZ_SECTOR_HIGH) {
        if (!model->command_committed) return 0;
        model->sector = (model->sector & 0x0000FFFFu) | ((uint32_t)(uint16_t)value << 16);
        model->sector_registers_dirty = 1;
        return 1;
    }
    if (address == EZ_COUNT) {
        if (!model->command_committed) return 0;
        model->count_word = (uint16_t)value;
        model->sector_registers_dirty = 1;
        return 1;
    }
    if (address == EZ_CONTROL) {
        if (!model->command_committed) return 0;
        model->control = (uint8_t)value;
        if (model->control == 3) model->status_poll_pending = 1;
        return 1;
    }
    if (address == EZ_COMMAND_END) {
        if (value == 0x1500u && model->command_committed &&
            model->sector_registers_dirty) {
            model->command_committed = 0;
            model->sector_registers_dirty = 0;
            (void)start_command(model);
        }
        model->command_committed = 0;
        model->unlock_stage = 0;
        return 1;
    }
    if (address < EZ_STATUS_DATA || address >= EZ_STATUS_DATA + 2048u ||
        (address & 1u)) return 0;
    if (model->command_active && !model->write_command) return 1;
    uint16_t offset = (uint16_t)(address - EZ_STATUS_DATA);
    unsigned bytes = width / 8u;
    if (offset + bytes > sizeof(model->buffer)) return 0;
    model->buffer[offset] = (uint8_t)value;
    model->buffer[offset + 1u] = (uint8_t)(value >> 8);
    if (width == 32) {
        model->buffer[offset + 2u] = (uint8_t)(value >> 16);
        model->buffer[offset + 3u] = (uint8_t)(value >> 24);
    }
    model->buffer_offset = (uint16_t)(offset + bytes);
    if (model->command_active && model->write_command &&
        model->buffer_offset == model->buffer_bytes) {
        if (fseek(model->image, (long)((uint64_t)model->sector * 512u), SEEK_SET) ||
            fwrite(model->buffer, 1, model->buffer_bytes, model->image) != model->buffer_bytes ||
            fflush(model->image)) {
            model->failed = 1;
            model->command_active = 0;
            ++model->failures;
            return 1;
        }
        (void)advance_block(model);
    }
    return 1;
}
