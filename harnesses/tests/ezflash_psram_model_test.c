#include "ezflash_psram_model.h"

#include <assert.h>
#include <stdio.h>

static void select_page(EZFlashPSRAMModel *model, uint16_t page) {
    assert(ezflash_psram_model_write(model, 0x09FE0000u, 16, 0xD200u));
    assert(!ezflash_psram_model_write(model, 0x08000000u, 16, 0x1500u));
    assert(!ezflash_psram_model_write(model, 0x08020000u, 16, 0xD200u));
    assert(!ezflash_psram_model_write(model, 0x08040000u, 16, 0x1500u));
    assert(ezflash_psram_model_write(model, 0x09860000u, 16, page));
    assert(ezflash_psram_model_write(model, 0x09FC0000u, 16, 0x1500u));
}

int main(void) {
    EZFlashPSRAMModel model = {0};
    uint32_t value = 0;
    assert(ezflash_psram_model_init(&model));

    /* A page register write without the source unlock transcript is ignored. */
    assert(!ezflash_psram_model_write(&model, 0x09860000u, 16, 0x0800u));
    assert(model.page == 0 && model.page_writes == 0);

    assert(ezflash_psram_model_write(&model, EZFLASH_PSRAM_BASE, 32,
                                     0x11223344u));
    select_page(&model, 0x0800u); /* 4 MiB window from source page step 0x0800. */
    assert(model.page == 0x0800u);
    assert(ezflash_psram_model_write(&model, EZFLASH_PSRAM_BASE, 32,
                                     0xA1B2C3D4u));
    select_page(&model, 0);
    assert(ezflash_psram_model_read(&model, EZFLASH_PSRAM_BASE, 32, &value));
    assert(value == 0x11223344u);
    select_page(&model, 0x0800u);
    assert(ezflash_psram_model_read(&model, EZFLASH_PSRAM_BASE, 32, &value));
    assert(value == 0xA1B2C3D4u);

    select_page(&model, 0x1000u); /* 8 MiB window from source page step 0x1000. */
    assert(ezflash_psram_model_write(&model, EZFLASH_PSRAM_BASE + 0x7FFFFCu,
                                     32, 0x55667788u));
    assert(ezflash_psram_model_read(&model, EZFLASH_PSRAM_BASE + 0x7FFFFCu,
                                    32, &value));
    assert(value == 0x55667788u);
    assert(model.out_of_range == 0 && model.page_writes == 4);

    ezflash_psram_model_deinit(&model);
    puts("EZ source-shaped PSRAM page model passed");
    return 0;
}
