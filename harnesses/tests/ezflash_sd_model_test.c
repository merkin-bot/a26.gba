#include "ezflash_sd_model.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STATUS 0x09E00000u

static void command(EZFlashSDModel *model, uint32_t sector, uint16_t count) {
    assert(ezflash_sd_model_write(model, 0x09FE0000u, 16, 0xD200));
    assert(!ezflash_sd_model_write(model, 0x08000000u, 16, 0x1500));
    assert(!ezflash_sd_model_write(model, 0x08020000u, 16, 0xD200));
    assert(!ezflash_sd_model_write(model, 0x08040000u, 16, 0x1500));
    assert(ezflash_sd_model_write(model, 0x09600000u, 16, sector));
    assert(ezflash_sd_model_write(model, 0x09620000u, 16, sector >> 16));
    assert(ezflash_sd_model_write(model, 0x09640000u, 16, count));
    assert(ezflash_sd_model_write(model, 0x09FC0000u, 16, 0x1500));
}

static void set_control(EZFlashSDModel *model, uint16_t control) {
    assert(ezflash_sd_model_write(model, 0x09FE0000u, 16, 0xD200));
    assert(!ezflash_sd_model_write(model, 0x08000000u, 16, 0x1500));
    assert(!ezflash_sd_model_write(model, 0x08020000u, 16, 0xD200));
    assert(!ezflash_sd_model_write(model, 0x08040000u, 16, 0x1500));
    assert(ezflash_sd_model_write(model, 0x09400000u, 16, control));
    assert(ezflash_sd_model_write(model, 0x09FC0000u, 16, 0x1500));
}

int main(void) {
    char path[] = "/tmp/ezflash-sd-XXXXXX";
    int fd = mkstemp(path);
    assert(fd >= 0);
    FILE *file = fdopen(fd, "w+b");
    assert(file);
    uint8_t source[4096];
    for (size_t i = 0; i < sizeof(source); ++i) source[i] = (uint8_t)(i * 17u);
    assert(fwrite(source, 1, sizeof(source), file) == sizeof(source));
    fclose(file);

    EZFlashSDModel model;
    assert(ezflash_sd_model_open(&model, path, 0));
    /* Missing an unlock step must not commit an apparent SD transaction. */
    assert(ezflash_sd_model_write(&model, 0x09FE0000u, 16, 0xD200));
    assert(!ezflash_sd_model_write(&model, 0x08000000u, 16, 0x0000));
    assert(!ezflash_sd_model_write(&model, 0x09600000u, 16, 1));
    assert(!ezflash_sd_model_write(&model, 0x09640000u, 16, 1));
    assert(ezflash_sd_model_write(&model, 0x09FC0000u, 16, 0x1500));
    assert(model.commands == 0);
    uint32_t value = 0;
    assert(!ezflash_sd_model_open(&(EZFlashSDModel){0}, path, 0xEEE1u));
    set_control(&model, 1);
    command(&model, 2, 2);
    set_control(&model, 3);
    assert(ezflash_sd_model_read(&model, STATUS, 16, &value) && value == 0);
    for (unsigned i = 0; i < 1024; i += 4) {
        assert(ezflash_sd_model_read(&model, STATUS + i, 32, &value));
        assert(value == ((uint32_t)source[1024 + i] |
                         (uint32_t)source[1025 + i] << 8 |
                         (uint32_t)source[1026 + i] << 16 |
                         (uint32_t)source[1027 + i] << 24));
    }
    assert(model.blocks_read == 2 && model.commands == 1 && model.failures == 0);

    /* The kernel DMA-writes payload before it emits the write command. */
    set_control(&model, 3);
    for (unsigned i = 0; i < 512; i += 4)
        assert(ezflash_sd_model_write(&model, STATUS + i, 32, 0xA5C30000u | i));
    command(&model, 3, 0x8001u);
    assert(model.blocks_written == 1 && model.failures == 0);
    command(&model, 8, 1); /* Image has eight sectors; end-of-image is rejected. */
    assert(!model.command_active && model.failures == 1);
    assert(ezflash_sd_model_read(&model, STATUS, 16, &value));
    ezflash_sd_model_close(&model);
    file = fopen(path, "rb");
    assert(file && fseek(file, 3 * 512, SEEK_SET) == 0);
    uint8_t written[512];
    assert(fread(written, 1, sizeof(written), file) == sizeof(written));
    fclose(file);
    for (unsigned i = 0; i < sizeof(written); i += 4) {
        assert(written[i] == (uint8_t)i && written[i + 1] == (uint8_t)(i >> 8));
        assert(written[i + 2] == 0xC3 && written[i + 3] == 0xA5);
    }
    EZFlashSDModel configured_status;
    assert(ezflash_sd_model_open(&configured_status, path, 0x1234u));
    set_control(&configured_status, 3);
    assert(ezflash_sd_model_read(&configured_status, STATUS, 16, &value));
    assert(value == 0x1234u);
    ezflash_sd_model_close(&configured_status);
    unlink(path);
    puts("EZ synthetic SD image model: 2-sector read, pre-command DMA write, status policy passed");
    return 0;
}
