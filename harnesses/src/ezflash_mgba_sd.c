#include "ezflash_mgba_sd.h"

#include <mgba/core/core.h>

#include <string.h>

static EZFlashMGBASD *active_adapter;

static uint32_t load_value(struct ARMCore *cpu, uint32_t address,
                           unsigned width, int *cycles,
                           uint32_t (*fallback)(struct ARMCore *, uint32_t, int *)) {
    /* Keep mGBA's GBA access timing/side effects even when synthetic card
     * contents replace the returned data. */
    uint32_t value = fallback(cpu, address, cycles);
    if (active_adapter) ++active_adapter->upstream_reads;
    if (active_adapter && ezflash_sd_model_read(active_adapter->model,
                                                 address, width, &value)) {
        ++active_adapter->synthetic_reads;
        return value;
    }
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
        ++active_adapter->delegated_writes;
    }
}
static void store16(struct ARMCore *cpu, uint32_t a, int16_t v, int *c) {
    active_adapter->store16(cpu, a, v, c);
    if (active_adapter) ++active_adapter->upstream_writes;
    if (active_adapter && ezflash_sd_model_write(active_adapter->model, a, 16,
                                                  (uint16_t)v)) {
        ++active_adapter->synthetic_writes;
        return;
    }
    if (active_adapter) ++active_adapter->delegated_writes;
}
static void store32(struct ARMCore *cpu, uint32_t a, int32_t v, int *c) {
    active_adapter->store32(cpu, a, v, c);
    if (active_adapter) ++active_adapter->upstream_writes;
    if (active_adapter && ezflash_sd_model_write(active_adapter->model, a, 32,
                                                  (uint32_t)v)) {
        ++active_adapter->synthetic_writes;
        return;
    }
    if (active_adapter) ++active_adapter->delegated_writes;
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
            if (ezflash_sd_model_read(active_adapter->model, address, 32, &value)) {
                cpu->gprs[reg] = value;
                ++active_adapter->synthetic_reads;
            }
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
            if (ezflash_sd_model_write(active_adapter->model, address, 32, value))
                ++active_adapter->synthetic_writes;
            address += (uint32_t)step;
        }
    }
    return next;
}

int ezflash_mgba_sd_attach(EZFlashMGBASD *adapter, struct mCore *core,
                           EZFlashSDModel *model) {
    if (!adapter || !core || !core->cpu || !model || active_adapter) return 0;
    memset(adapter, 0, sizeof(*adapter));
    adapter->core = core;
    adapter->model = model;
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
    active_adapter = NULL;
    memset(adapter, 0, sizeof(*adapter));
}
