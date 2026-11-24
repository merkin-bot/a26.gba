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

void supercard_reset(SuperCardModel *model) {
    model->mode = SUPERCARD_MODE_FIRMWARE;
    model->sd_interface = 0;
    model->write_access = 0;
    model->mode_magic_writes = 0;
    model->mode_value_writes = 0;
    model->pending_mode = 0;
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
    if (!model || !value || !model->sd_interface || width != 16) {
        return 0;
    }
    /* DSSMM supercard_io.S polls CMD bit0 and DAT0 bit8 for idle-high. */
    if (address == SC_RDWR_COMMAND) {
        *value = 0x0001u;
        return 1;
    }
    if (address == SC_WRITE_REGISTER_8 || address == SC_WRITE_REGISTER_A ||
        address == SC_READ_REGISTER_8 || address == SC_READ_REGISTER_A) {
        *value = 0x0100u;
        return 1;
    }
    return 0;
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
