#include "ezflash_nor_model.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define U1 (EZFLASH_S71_NOR_BASE + 0x555u * 2u)
#define U2 (EZFLASH_S71_NOR_BASE + 0x2AAu * 2u)

int main(void) {
    char path[] = "/tmp/ezflash-nor-kernel-XXXXXX";
    int fd = mkstemp(path);
    assert(fd >= 0);
    FILE *file = fdopen(fd, "wb");
    assert(file);
    const uint8_t kernel[] = {0x12, 0x34, 0x56, 0x78};
    assert(fwrite(kernel, 1, sizeof(kernel), file) == sizeof(kernel));
    assert(fclose(file) == 0);

    EZFlashNORModel model = {0};
    uint32_t value = 0;
    assert(ezflash_nor_model_init(&model, path));
    assert(ezflash_nor_model_read(&model, EZFLASH_S71_NOR_BASE, 32, &value));
    assert(value == 0x78563412u);

    /* Source S71 autoselect transcript and source-selected PL064 device ID. */
    assert(ezflash_nor_model_write(&model, EZFLASH_S71_NOR_BASE, 16, 0xF0));
    assert(!ezflash_nor_model_write(&model, U1, 16, 0xAA));
    assert(!ezflash_nor_model_write(&model, U2, 16, 0x55));
    assert(!ezflash_nor_model_write(&model, U1, 16, 0x90));
    assert(ezflash_nor_model_read(&model, EZFLASH_S71_NOR_BASE + 0x1Cu,
                                  16, &value));
    assert(value == 0x2202u);
    assert(ezflash_nor_model_write(&model, EZFLASH_S71_NOR_BASE, 16, 0xF0));

    /* Source sector erase then word-program sequence used by Save_info(). */
    uint32_t target = EZFLASH_S71_NOR_BASE + 0x790000u;
    assert(!ezflash_nor_model_write(&model, U1, 16, 0xAA));
    assert(!ezflash_nor_model_write(&model, U2, 16, 0x55));
    assert(!ezflash_nor_model_write(&model, U1, 16, 0x80));
    assert(!ezflash_nor_model_write(&model, U1, 16, 0xAA));
    assert(!ezflash_nor_model_write(&model, U2, 16, 0x55));
    assert(ezflash_nor_model_write(&model, target, 16, 0x30));
    assert(ezflash_nor_model_read(&model, target, 16, &value) && value == 0xFFFFu);
    assert(!ezflash_nor_model_write(&model, U1, 16, 0xAA));
    assert(!ezflash_nor_model_write(&model, U2, 16, 0x55));
    assert(!ezflash_nor_model_write(&model, U1, 16, 0xA0));
    assert(ezflash_nor_model_write(&model, target, 16, 0x1234u));
    assert(ezflash_nor_model_read(&model, target, 16, &value) && value == 0x1234u);
    assert(model.erase_commands == 1 && model.program_words == 1);

    ezflash_nor_model_deinit(&model);
    unlink(path);
    puts("EZ source-shaped S71 NOR ID/erase/program model passed");
    return 0;
}
