#include "ezflash_psram_model.h"

#include <stdlib.h>
#include <string.h>

#define EZ_COMMAND_END 0x09FC0000u
#define EZ_COMMAND_UNLOCK 0x09FE0000u
#define EZ_PSRAM_PAGE 0x09860000u
#define EZ_ROM_PAGE 0x09880000u

int ezflash_psram_model_init(EZFlashPSRAMModel *model) {
    if (!model) return 0;
    memset(model, 0, sizeof(*model));
    model->bytes = calloc(1, EZFLASH_PSRAM_CAPACITY_BYTES);
    return model->bytes != NULL;
}

void ezflash_psram_model_deinit(EZFlashPSRAMModel *model) {
    if (!model) return;
    free(model->bytes);
    memset(model, 0, sizeof(*model));
}

static int psram_offset(EZFlashPSRAMModel *model, uint32_t address,
                        unsigned width, uint32_t *offset) {
    if (!model || !model->bytes || !offset ||
        (width != 8 && width != 16 && width != 32) ||
        (address & ((width / 8u) - 1u))) return 0;
    uint64_t physical;
    if (address >= EZFLASH_PSRAM_BASE &&
        address < EZFLASH_PSRAM_BASE + EZFLASH_PSRAM_WINDOW_BYTES) {
        physical = ((uint64_t)model->page << 11) +
                   (address - EZFLASH_PSRAM_BASE);
    } else if (((model->rom_page & (uint16_t)~1u) == 0x0200u ||
                (model->rom_page & (uint16_t)~1u) == 0x0400u) &&
               address >= 0x08000000u && address < 0x08800000u) {
        physical = address - 0x08000000u;
    } else {
        return 0;
    }
    if (physical + width / 8u > EZFLASH_PSRAM_CAPACITY_BYTES) {
        ++model->out_of_range;
        return 0;
    }
    *offset = (uint32_t)physical;
    return 1;
}

int ezflash_psram_model_read(EZFlashPSRAMModel *model, uint32_t address,
                             unsigned width, uint32_t *value) {
    uint32_t offset;
    if (!value || !psram_offset(model, address, width, &offset)) return 0;
    *value = model->bytes[offset];
    if (width >= 16) *value |= (uint32_t)model->bytes[offset + 1u] << 8;
    if (width == 32) {
        *value |= (uint32_t)model->bytes[offset + 2u] << 16;
        *value |= (uint32_t)model->bytes[offset + 3u] << 24;
    }
    ++model->reads;
    return 1;
}

int ezflash_psram_model_write(EZFlashPSRAMModel *model, uint32_t address,
                              unsigned width, uint32_t value) {
    if (!model || !model->bytes) return 0;
    if (address == EZ_COMMAND_UNLOCK && width == 16) {
        model->unlock_stage = value == 0xD200u ? 1u : 0u;
        model->page_committed = 0;
        return 1;
    }
    if (address == 0x08000000u && width == 16) {
        model->unlock_stage = model->unlock_stage == 1u && value == 0x1500u
            ? 2u : 0u;
        return 0;
    }
    if (address == 0x08020000u && width == 16) {
        model->unlock_stage = model->unlock_stage == 2u && value == 0xD200u
            ? 3u : 0u;
        return 0;
    }
    if (address == 0x08040000u && width == 16) {
        model->unlock_stage = model->unlock_stage == 3u && value == 0x1500u
            ? 4u : 0u;
        model->page_committed = 0;
        return 0;
    }
    if ((address == EZ_PSRAM_PAGE || address == EZ_ROM_PAGE) && width == 16) {
        if (model->unlock_stage != 4u) return 0;
        if (address == EZ_PSRAM_PAGE) {
            model->page = (uint16_t)value;
            ++model->page_writes;
        } else {
            model->rom_page = (uint16_t)value;
            ++model->rom_page_writes;
        }
        model->page_committed = 1;
        return 1;
    }
    if (address == EZ_COMMAND_END && width == 16) {
        model->unlock_stage = 0;
        model->page_committed = 0;
        return 1;
    }
    uint32_t offset;
    if (!psram_offset(model, address, width, &offset)) return 0;
    model->bytes[offset] = (uint8_t)value;
    if (width >= 16) model->bytes[offset + 1u] = (uint8_t)(value >> 8);
    if (width == 32) {
        model->bytes[offset + 2u] = (uint8_t)(value >> 16);
        model->bytes[offset + 3u] = (uint8_t)(value >> 24);
    }
    ++model->writes;
    return 1;
}
