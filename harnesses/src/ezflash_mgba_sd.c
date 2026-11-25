#include "ezflash_mgba_sd.h"

#include <mgba/core/core.h>
#include <mgba/internal/gba/gba.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static EZFlashMGBASD *active_adapter;

static int synthetic_read(uint32_t address, unsigned width, uint32_t *value) {
    if (active_adapter && ezflash_sd_model_read(active_adapter->model,
                                                 address, width, value)) {
        ++active_adapter->synthetic_reads;
        if (getenv("A26_EZ_TRACE_DATA") &&
            address >= 0x09E00000u && address < 0x09E00100u &&
            (active_adapter->model->last_command_sector == 2051u ||
             active_adapter->model->last_command_sector == 2052u ||
             active_adapter->model->last_command_sector == 2054u)) {
            fprintf(stderr, "ez_sd_data lba=%u offset=%03X width=%u value=%08X\n",
                    active_adapter->model->last_command_sector,
                    address - 0x09E00000u, width, *value);
        }
        return 1;
    }
    if (active_adapter && active_adapter->psram &&
        ezflash_psram_model_read(active_adapter->psram, address, width, value)) {
        ++active_adapter->psram_reads;
        return 1;
    }
    if (active_adapter && active_adapter->nor &&
        ezflash_nor_model_read(active_adapter->nor, address, width, value)) {
        ++active_adapter->nor_reads;
        return 1;
    }
    if (active_adapter && active_adapter->nor_s98 &&
        ezflash_nor_model_read(active_adapter->nor_s98, address, width, value)) {
        ++active_adapter->nor_s98_reads;
        return 1;
    }
    return 0;
}

static int synthetic_write(uint32_t address, unsigned width, uint32_t value) {
    int handled = 0;
    if (active_adapter && ezflash_sd_model_write(active_adapter->model,
                                                  address, width, value)) {
        ++active_adapter->synthetic_writes;
        handled = 1;
    }
    if (active_adapter && active_adapter->psram) {
        int psram_handled = ezflash_psram_model_write(active_adapter->psram,
                                                       address, width, value);
        if (psram_handled) {
            ++active_adapter->psram_writes;
            handled = 1;
        }
        /* mGBA fetches GBA instructions directly from GBAMemory.rom, so its
         * data-bus callbacks cannot expose a page-selected cartridge image
         * to the CPU. Mirror the selected PSRAM ROM window into that buffer
         * only after the source unlock transaction commits. */
        if (address == 0x09FC0000u && width == 16 && value == 0x1500u &&
            active_adapter->core && active_adapter->core->board &&
            active_adapter->rom_snapshot) {
            struct GBA *gba = active_adapter->core->board;
            uint16_t rom_page = active_adapter->psram->rom_page & (uint16_t)~1u;
            if (rom_page == 0x0200u || rom_page == 0x0400u) {
                size_t size = active_adapter->rom_snapshot_size;
                if (size > EZFLASH_PSRAM_CAPACITY_BYTES)
                    size = EZFLASH_PSRAM_CAPACITY_BYTES;
                memcpy(gba->memory.rom, active_adapter->psram->bytes, size);
            } else {
                memcpy(gba->memory.rom, active_adapter->rom_snapshot,
                       active_adapter->rom_snapshot_size);
            }
        }
    }
    if (active_adapter && active_adapter->nor &&
        ezflash_nor_model_write(active_adapter->nor, address, width, value)) {
        ++active_adapter->nor_writes;
        handled = 1;
    }
    if (active_adapter && active_adapter->nor_s98 &&
        ezflash_nor_model_write(active_adapter->nor_s98, address, width, value)) {
        ++active_adapter->nor_s98_writes;
        handled = 1;
    }
    return handled;
}

static uint32_t load_value(struct ARMCore *cpu, uint32_t address,
                           unsigned width, int *cycles,
                           uint32_t (*fallback)(struct ARMCore *, uint32_t, int *)) {
    /* Keep mGBA's GBA access timing/side effects even when synthetic card
     * contents replace the returned data. */
    uint32_t value = fallback(cpu, address, cycles);
    if (active_adapter) ++active_adapter->upstream_reads;
    if (synthetic_read(address, width, &value)) return value;
    if (active_adapter) ++active_adapter->delegated_reads;
    return value;
}

static uint32_t load8(struct ARMCore *cpu, uint32_t a, int *c) {
    return load_value(cpu, a, 8, c, active_adapter->load8);
}
static uint32_t load16(struct ARMCore *cpu, uint32_t a, int *c) {
    return load_value(cpu, a, 16, c, active_adapter->load16);
}
static uint32_t load32(struct ARMCore *cpu, uint32_t a, int *c) {
    return load_value(cpu, a, 32, c, active_adapter->load32);
}

static void store8(struct ARMCore *cpu, uint32_t a, int8_t v, int *c) {
    active_adapter->store8(cpu, a, v, c);
    if (active_adapter) {
        ++active_adapter->upstream_writes;
        if (!synthetic_write(a, 8, (uint8_t)v))
            ++active_adapter->delegated_writes;
    }
}
static void store16(struct ARMCore *cpu, uint32_t a, int16_t v, int *c) {
    active_adapter->store16(cpu, a, v, c);
    if (active_adapter) ++active_adapter->upstream_writes;
    if (!synthetic_write(a, 16, (uint16_t)v) && active_adapter)
        ++active_adapter->delegated_writes;
}
static void store32(struct ARMCore *cpu, uint32_t a, int32_t v, int *c) {
    active_adapter->store32(cpu, a, v, c);
    if (active_adapter) ++active_adapter->upstream_writes;
    if (!synthetic_write(a, 32, (uint32_t)v) && active_adapter)
        ++active_adapter->delegated_writes;
}

static uint32_t multi_address(uint32_t address, int mask,
                              enum LSMDirection direction) {
    unsigned count = 0;
    for (unsigned reg = 0; reg < 16; ++reg) count += (mask >> reg) & 1;
    if (direction & LSM_D) address -= (count << 2) - 4u;
    if (direction & LSM_B) address += (direction & LSM_D) ? (uint32_t)-4 : 4u;
    return address;
}

static uint32_t load_multiple(struct ARMCore *cpu, uint32_t address, int mask,
                              enum LSMDirection direction, int *cycles) {
    uint32_t next = active_adapter->load_multiple(cpu, address, mask,
                                                  direction, cycles);
    if (active_adapter) {
        ++active_adapter->multi_loads;
        address = multi_address(address, mask, direction);
        int step = (direction & LSM_D) ? -4 : 4;
        for (unsigned reg = 0; reg < 16; ++reg) if (mask & (1 << reg)) {
            uint32_t value;
            if (synthetic_read(address, 32, &value)) cpu->gprs[reg] = value;
            address += (uint32_t)step;
        }
    }
    return next;
}

static uint32_t store_multiple(struct ARMCore *cpu, uint32_t address, int mask,
                               enum LSMDirection direction, int *cycles) {
    uint32_t next = active_adapter->store_multiple(cpu, address, mask,
                                                    direction, cycles);
    if (active_adapter) {
        ++active_adapter->multi_stores;
        address = multi_address(address, mask, direction);
        int step = (direction & LSM_D) ? -4 : 4;
        for (unsigned reg = 0; reg < 16; ++reg) if (mask & (1 << reg)) {
            uint32_t value = cpu->gprs[reg];
            if (reg == ARM_PC) value += WORD_SIZE_ARM;
            (void)synthetic_write(address, 32, value);
            address += (uint32_t)step;
        }
    }
    return next;
}

int ezflash_mgba_sd_attach(EZFlashMGBASD *adapter, struct mCore *core,
                           EZFlashSDModel *model, EZFlashPSRAMModel *psram,
                           EZFlashNORModel *nor, EZFlashNORModel *nor_s98) {
    if (!adapter || !core || !core->cpu || !model || !psram || !nor ||
        !nor_s98 || active_adapter)
        return 0;
    memset(adapter, 0, sizeof(*adapter));
    adapter->core = core;
    adapter->model = model;
    adapter->psram = psram;
    adapter->nor = nor;
    adapter->nor_s98 = nor_s98;
    struct GBA *gba = core->board;
    if (!gba || !gba->memory.rom || !gba->memory.romSize) {
        memset(adapter, 0, sizeof(*adapter));
        return 0;
    }
    adapter->rom_snapshot_size = gba->memory.romSize;
    adapter->rom_snapshot = malloc(adapter->rom_snapshot_size);
    if (!adapter->rom_snapshot) {
        memset(adapter, 0, sizeof(*adapter));
        return 0;
    }
    memcpy(adapter->rom_snapshot, gba->memory.rom, adapter->rom_snapshot_size);
    struct ARMCore *cpu = core->cpu;
    adapter->load8 = cpu->memory.load8;
    adapter->load16 = cpu->memory.load16;
    adapter->load32 = cpu->memory.load32;
    adapter->load_multiple = cpu->memory.loadMultiple;
    adapter->store8 = cpu->memory.store8;
    adapter->store16 = cpu->memory.store16;
    adapter->store32 = cpu->memory.store32;
    adapter->store_multiple = cpu->memory.storeMultiple;
    if (!adapter->load8 || !adapter->load16 || !adapter->load32 ||
        !adapter->load_multiple || !adapter->store8 || !adapter->store16 ||
        !adapter->store32 || !adapter->store_multiple) {
        free(adapter->rom_snapshot);
        memset(adapter, 0, sizeof(*adapter));
        return 0;
    }
    active_adapter = adapter;
    cpu->memory.load8 = load8;
    cpu->memory.load16 = load16;
    cpu->memory.load32 = load32;
    cpu->memory.loadMultiple = load_multiple;
    cpu->memory.store8 = store8;
    cpu->memory.store16 = store16;
    cpu->memory.store32 = store32;
    cpu->memory.storeMultiple = store_multiple;
    return 1;
}

void ezflash_mgba_sd_detach(EZFlashMGBASD *adapter) {
    if (!adapter || active_adapter != adapter || !adapter->core ||
        !adapter->core->cpu) return;
    struct ARMCore *cpu = adapter->core->cpu;
    cpu->memory.load8 = adapter->load8;
    cpu->memory.load16 = adapter->load16;
    cpu->memory.load32 = adapter->load32;
    cpu->memory.loadMultiple = adapter->load_multiple;
    cpu->memory.store8 = adapter->store8;
    cpu->memory.store16 = adapter->store16;
    cpu->memory.store32 = adapter->store32;
    cpu->memory.storeMultiple = adapter->store_multiple;
    if (adapter->rom_snapshot && adapter->core && adapter->core->board) {
        struct GBA *gba = adapter->core->board;
        memcpy(gba->memory.rom, adapter->rom_snapshot,
               adapter->rom_snapshot_size);
        free(adapter->rom_snapshot);
    }
    active_adapter = NULL;
    memset(adapter, 0, sizeof(*adapter));
}
