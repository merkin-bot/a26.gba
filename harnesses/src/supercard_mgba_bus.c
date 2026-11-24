#include "supercard_mgba_bus.h"

#include <mgba/core/core.h>
#include <mgba/internal/arm/arm.h>

static SuperCardMGBABus *active_bus;

static uint32_t supercard_load_multiple(struct ARMCore *cpu, uint32_t address,
                                        int mask, enum LSMDirection direction,
                                        int *cycle_counter) {
    if (active_bus && direction == LSM_IA && address >= 0x09200000u &&
        address <= 0x0920001Cu) {
        uint32_t next = active_bus->original_load_multiple(
            cpu, address, mask, direction, cycle_counter);
        uint32_t data_address = address;
        for (unsigned reg = 0; reg < 16; ++reg) {
            if (!(mask & (1 << reg))) {
                continue;
            }
            uint32_t value;
            if (supercard_read(active_bus->model, data_address, 32, &value)) {
                cpu->gprs[reg] = value;
                ++active_bus->sd_data_reads;
            }
            data_address += 4u;
        }
        return next;
    }
    return active_bus->original_load_multiple(cpu, address, mask, direction,
                                               cycle_counter);
}

static uint32_t supercard_load32(struct ARMCore *cpu, uint32_t address,
                                 int *cycle_counter) {
    uint32_t value;
    if (active_bus && supercard_read(active_bus->model, address, 32, &value)) {
        if (address >= 0x09200000u && address <= 0x0920001Cu) {
            ++active_bus->sd_data_reads;
        }
        return value;
    }
    return active_bus->original_load32(cpu, address, cycle_counter);
}

static uint32_t supercard_load16(struct ARMCore *cpu, uint32_t address,
                                 int *cycle_counter) {
    uint32_t value;
    if (active_bus && supercard_read(active_bus->model, address, 16, &value)) {
        return value;
    }
    return active_bus->original_load16(cpu, address, cycle_counter);
}

static void count_sd_write(uint32_t address) {
    if (!active_bus) {
        return;
    }
    if (address >= 0x08000000u && address < 0x0E000000u) {
        ++active_bus->cartridge_writes;
        active_bus->last_cartridge_write = address;
    }
    /* Addresses are from DSSMM SuperFW src/supercard_io.S, Build769. */
    if (address == 0x09800000u) {
        ++active_bus->sd_command_writes;
    }
    if (address == 0x09000000u || address == 0x0B000000u ||
        address == 0x09200000u) {
        ++active_bus->sd_data_writes;
    }
}

static void supercard_store8(struct ARMCore *cpu, uint32_t address,
                             int8_t value, int *cycle_counter) {
    count_sd_write(address);
    active_bus->original_store8(cpu, address, value, cycle_counter);
}

static void supercard_store16(struct ARMCore *cpu, uint32_t address,
                              int16_t value, int *cycle_counter) {
    if (active_bus && address == 0x09FFFFFEu) {
        (void) supercard_write(active_bus->model, address, (uint16_t) value);
        ++active_bus->mode_register_writes;
        if ((uint16_t) value == 0xA55Au) {
            ++active_bus->mode_magic_writes;
        } else {
            ++active_bus->mode_value_writes;
            active_bus->last_mode_word = (uint16_t) value;
        }
        return;
    }
    if (active_bus) {
        (void) supercard_sd_command_write(active_bus->model, address, 16,
                                          (uint16_t)value);
    }
    if (active_bus && address == 0x09800000u &&
        active_bus->cmd_write_trace_count < SUPERCARD_CMD_TRACE_CAPACITY) {
        active_bus->cmd_write_trace[active_bus->cmd_write_trace_count++] =
            (uint16_t) value;
    }
    count_sd_write(address);
    active_bus->original_store16(cpu, address, value, cycle_counter);
}

static void supercard_store32(struct ARMCore *cpu, uint32_t address,
                              int32_t value, int *cycle_counter) {
    count_sd_write(address);
    active_bus->original_store32(cpu, address, value, cycle_counter);
}

int supercard_mgba_bus_attach(SuperCardMGBABus *bus, struct mCore *core,
                              SuperCardModel *model) {
    if (!bus || !core || !core->cpu || !model || active_bus) {
        return 0;
    }
    struct ARMCore *cpu = core->cpu;
    bus->core = core;
    bus->model = model;
    bus->original_load_multiple = cpu->memory.loadMultiple;
    bus->original_load32 = cpu->memory.load32;
    bus->original_load16 = cpu->memory.load16;
    bus->original_store8 = cpu->memory.store8;
    bus->original_store16 = cpu->memory.store16;
    bus->original_store32 = cpu->memory.store32;
    if (!bus->original_load_multiple || !bus->original_load32 ||
        !bus->original_load16 || !bus->original_store8 ||
        !bus->original_store16 || !bus->original_store32) {
        bus->core = NULL;
        bus->model = NULL;
        return 0;
    }
    active_bus = bus;
    cpu->memory.loadMultiple = supercard_load_multiple;
    cpu->memory.load32 = supercard_load32;
    cpu->memory.load16 = supercard_load16;
    cpu->memory.store8 = supercard_store8;
    cpu->memory.store16 = supercard_store16;
    cpu->memory.store32 = supercard_store32;
    return 1;
}

void supercard_mgba_bus_detach(SuperCardMGBABus *bus) {
    if (!bus || active_bus != bus || !bus->core || !bus->core->cpu) {
        return;
    }
    struct ARMCore *cpu = bus->core->cpu;
    cpu->memory.loadMultiple = bus->original_load_multiple;
    cpu->memory.load32 = bus->original_load32;
    cpu->memory.load16 = bus->original_load16;
    cpu->memory.store8 = bus->original_store8;
    cpu->memory.store16 = bus->original_store16;
    cpu->memory.store32 = bus->original_store32;
    active_bus = NULL;
    bus->core = NULL;
    bus->model = NULL;
    bus->original_load16 = NULL;
    bus->original_load32 = NULL;
    bus->original_load_multiple = NULL;
    bus->original_store8 = NULL;
    bus->original_store16 = NULL;
    bus->original_store32 = NULL;
}
