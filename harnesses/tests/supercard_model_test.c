#include "supercard_model.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    uint8_t cart[32], sdram[65536];
    memset(cart, 0xCC, sizeof(cart));
    memset(sdram, 0, sizeof(sdram));
    SuperCardModel model = {cart, sizeof(cart), sdram, sizeof(sdram), NULL,
                            SUPERCARD_MODE_FIRMWARE, 0, 0, 0, 0, 0};
    SuperCardModel second = {cart, sizeof(cart), sdram, sizeof(sdram), NULL,
                             SUPERCARD_MODE_FIRMWARE, 0, 0, 0, 0, 0};
    supercard_reset(&model);
    assert(model.mode == SUPERCARD_MODE_FIRMWARE);

    assert(supercard_write(&model, 0x09FFFFFEu, 0xA55A));
    assert(supercard_write(&second, 0x09FFFFFEu, 0xA55A));
    assert(supercard_write(&model, 0x09FFFFFEu, 0xA55A));
    assert(supercard_write(&second, 0x09FFFFFEu, 0xA55A));
    assert(supercard_write(&model, 0x09FFFFFEu, 0x0007));
    assert(model.mode == SUPERCARD_MODE_FIRMWARE); /* Mode applies on the second matching write. */
    assert(supercard_write(&second, 0x09FFFFFEu, 0x0007));
    assert(supercard_write(&second, 0x09FFFFFEu, 0x0007));
    assert(second.mode == SUPERCARD_MODE_SDRAM); /* First model's state is per-card. */
    assert(supercard_write(&model, 0x09FFFFFEu, 0x0007));
    assert(model.mode == SUPERCARD_MODE_SDRAM);
    assert(model.sd_interface);
    assert(model.write_access);
    uint32_t line_value = 0;
    assert(supercard_read(&model, 0x09800000u, 16, &line_value));
    assert(line_value == 0x0001u); /* CMD is documented idle-high. */
    assert(supercard_read(&model, 0x09100000u, 16, &line_value));
    assert(line_value == 0x0100u); /* DAT0 is documented idle-high. */
    assert(supercard_read(&model, 0x09000000u, 16, &line_value));
    assert(line_value == 0x0100u);
    assert(!supercard_read(&model, 0x09800000u, 8, &line_value));

    const uint8_t payload[] = {1, 2, 3, 4};
    supercard_set_payload(&model, 4096, payload, sizeof(payload));
    assert(memcmp(sdram + 4096, payload, sizeof(payload)) == 0);

    const char *sd = "/tmp/a26-supercard-model.img";
    FILE *file = fopen(sd, "wb");
    assert(file);
    uint8_t block[512];
    for (size_t i = 0; i < sizeof(block); ++i)
        block[i] = (uint8_t)i;
    assert(fwrite(block, 1, sizeof(block), file) == sizeof(block));
    fclose(file);
    model.sd_path = sd;
    uint8_t read_block[512];
    model.sd_interface = 0;
    assert(!supercard_sd_read(&model, 0, read_block, 1));
    model.sd_interface = 1;
    assert(supercard_write(&model, 0x09FFFFFEu, 0xA55A));
    assert(supercard_write(&model, 0x09FFFFFEu, 0xA55A));
    assert(supercard_write(&model, 0x09FFFFFEu, 0x0007));
    assert(supercard_write(&model, 0x09FFFFFEu, 0x0007));
    assert(supercard_sd_read(&model, 0, read_block, 1));
    assert(memcmp(read_block, block, sizeof(block)) == 0);
    read_block[0] ^= 0xFF;
    assert(supercard_sd_write(&model, 0, read_block, 1));
    assert(supercard_sd_read(&model, 0, read_block, 1));
    assert(read_block[0] == (uint8_t)(255 ^ 0));
    remove(sd);

    puts("SuperCard mode, SDRAM, and SD block model passed");
    return 0;
}
