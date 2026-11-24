#include "supercard_model.h"
#include <stdio.h>
#include <string.h>

#define SC_MODE_REG 0x09FFFFFEu
#define SC_MAGIC 0xA55Au

static int s_magic;

void supercard_reset(SuperCardModel *model) {
    model->mode = SUPERCARD_MODE_FIRMWARE;
    model->sd_interface = 0;
    model->write_access = 0;
    s_magic = 0;
}

int supercard_write(SuperCardModel *model, uint32_t address, uint16_t value) {
    if (address != SC_MODE_REG)
        return 0;
    if (s_magic) {
        s_magic = 0;
        model->mode = (value & 1u) ? SUPERCARD_MODE_SDRAM : SUPERCARD_MODE_FIRMWARE;
        model->sd_interface = (value & 2u) != 0;
        model->write_access = (value & 4u) != 0;
        return 1;
    }
    s_magic = value == SC_MAGIC;
    return 1;
}

int supercard_read(SuperCardModel *model, uint32_t address, uint32_t width,
                   uint32_t *value) {
    (void)model;
    (void)address;
    (void)width;
    (void)value;
    return 0;
}

void supercard_set_payload(SuperCardModel *model, uint32_t offset,
                           const uint8_t *data, size_t size) {
    if (!model->sdram || offset > model->sdram_size)
        return;
    size_t available = model->sdram_size - offset;
    if (size > available)
        size = available;
    memcpy(model->sdram + offset, data, size);
}

int supercard_sd_read(SuperCardModel *model, uint32_t lba,
                      uint8_t *buffer, size_t blocks) {
    if (!model->sd_path || !buffer)
        return 0;
    FILE *file = fopen(model->sd_path, "rb");
    if (!file)
        return 0;
    size_t bytes = blocks * 512u;
    if (fseek(file, (long)lba * 512l, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    size_t count = fread(buffer, 1, bytes, file);
    fclose(file);
    return count == bytes;
}

int supercard_sd_write(SuperCardModel *model, uint32_t lba,
                       const uint8_t *buffer, size_t blocks) {
    if (!model->sd_path || !buffer)
        return 0;
    FILE *file = fopen(model->sd_path, "r+b");
    if (!file)
        return 0;
    size_t bytes = blocks * 512u;
    if (fseek(file, (long)lba * 512l, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    size_t count = fwrite(buffer, 1, bytes, file);
    fclose(file);
    return count == bytes;
}
