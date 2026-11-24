#include "supercard_mgba_bus.h"

#include <mgba/core/core.h>
#include <mgba/internal/arm/arm.h>
#include <mgba/internal/gba/gba.h>

static SuperCardMGBABus *active_bus;

static void set_cartridge_fetch_map(SuperCardMGBABus *bus, uint32_t *rom,
                                    size_t rom_size, uint32_t rom_mask) {
    struct GBA *gba = bus->core->board;
    uint32_t *old_rom = gba->memory.rom;
    gba->memory.rom = rom;
    gba->memory.romSize = rom_size;
    gba->memory.romMask = rom_mask;
    if (gba->cpu && gba->memory.activeRegion >= GBA_REGION_ROM0 &&
        gba->memory.activeRegion <= GBA_REGION_ROM2_EX &&
        (old_rom == gba->cpu->memory.activeRegion ||
         gba->cpu->memory.activeRegion == bus->original_rom ||
         gba->cpu->memory.activeRegion == (uint32_t *)bus->model->sdram)) {
        gba->cpu->memory.activeRegion = gba->memory.rom;
        gba->cpu->memory.activeMask = gba->memory.romMask &
            -(gba->cpu->cpsr.t ? WORD_SIZE_THUMB : WORD_SIZE_ARM);
    }
}

static void sync_cartridge_fetch_map(SuperCardMGBABus *bus) {
    if (!bus || !bus->core || !bus->core->board || !bus->model) {
        return;
    }
    if (bus->model->mode == SUPERCARD_MODE_SDRAM) {
        set_cartridge_fetch_map(bus, (uint32_t *)bus->model->sdram,
                                bus->model->sdram_size,
                                (uint32_t)(bus->model->sdram_size - 1u));
    } else {
        set_cartridge_fetch_map(bus, bus->original_rom,
                                bus->original_rom_size,
                                bus->original_rom_mask);
    }
}

static int is_sdram_address(uint32_t address) {
    return address >= 0x08000000u && address < 0x0A000000u;
}

static int is_lite_data_address(uint32_t address) {
    return address >= 0x09200000u && address <= 0x0920001Cu;
}

static void trace_reset_stack_write(struct ARMCore *cpu, uint32_t address,
                                    uint8_t width, uint32_t value) {
    if (!active_bus || address < 0x03000000u || address >= 0x04000000u ||
        (address & 0x7FFFu) != 0x7FECu) {
        return;
    }
    uint32_t index = active_bus->reset_stack_trace_total %
                     SUPERCARD_RESET_TRACE_CAPACITY;
    active_bus->reset_stack_trace[index] = (SuperCardResetStackWrite) {
        .pc = cpu->gprs[ARM_PC],
        .address = address,
        .value = value,
        .width = width,
    };
    ++active_bus->reset_stack_trace_total;
    if (active_bus->reset_stack_trace_count < SUPERCARD_RESET_TRACE_CAPACITY) {
        ++active_bus->reset_stack_trace_count;
    }
}

static void trace_reset_stack_multiple(struct ARMCore *cpu, uint32_t address,
                                       int mask,
                                       enum LSMDirection direction) {
    int offset = 4;
    int registers = 0;
    for (unsigned reg = 0; reg < 16; ++reg) {
        registers += (mask >> reg) & 1;
    }
    if (direction & LSM_D) {
        offset = -4;
        address -= ((uint32_t)registers << 2) - 4u;
    }
    if (direction & LSM_B) {
        address += (uint32_t)offset;
    }
    for (unsigned reg = 0; reg < 16; ++reg) {
        if (!(mask & (1 << reg))) {
            continue;
        }
        uint32_t value = cpu->gprs[reg];
        if (reg == ARM_PC) {
            value += WORD_SIZE_ARM;
        }
        trace_reset_stack_write(cpu, address, 32, value);
        address += 4u;
    }
}

static int is_sdram_multiple(struct ARMCore *cpu, uint32_t address,
                             int mask, enum LSMDirection direction) {
    (void)cpu;
    (void)mask;
    return direction == LSM_IA && is_sdram_address(address);
}

static int is_lite_data_multiple(uint32_t address, int mask,
                                enum LSMDirection direction) {
    return direction == LSM_IA && address == 0x09200000u && mask != 0;
}

static uint32_t supercard_load_multiple(struct ARMCore *cpu, uint32_t address,
                                        int mask, enum LSMDirection direction,
                                        int *cycle_counter) {
    if (active_bus && direction == LSM_IA &&
        ((address >= 0x09200000u && address <= 0x0920001Cu) ||
         is_sdram_address(address))) {
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
                if (data_address >= 0x09200000u && data_address <= 0x0920001Cu) {
                    ++active_bus->sd_data_reads;
                }
            }
            data_address += 4u;
        }
        return next;
    }
    return active_bus->original_load_multiple(cpu, address, mask, direction,
                                               cycle_counter);
}

static uint32_t supercard_store_multiple(struct ARMCore *cpu, uint32_t address,
                                         int mask, enum LSMDirection direction,
                                         int *cycle_counter) {
    if (active_bus && (is_sdram_multiple(cpu, address, mask, direction) ||
                       is_lite_data_multiple(address, mask, direction))) {
        uint32_t next = active_bus->original_store_multiple(
            cpu, address, mask, direction, cycle_counter);
        trace_reset_stack_multiple(cpu, address, mask, direction);
        uint32_t data_address = address;
        for (unsigned reg = 0; reg < 16; ++reg) {
            if (!(mask & (1 << reg))) {
                continue;
            }
            if (is_lite_data_address(data_address) ||
                !supercard_store_memory(active_bus->model, data_address, 32,
                                        cpu->gprs[reg])) {
                (void)supercard_sd_data_write(active_bus->model, data_address,
                                              32, cpu->gprs[reg]);
            }
            data_address += 4u;
        }
        return next;
    }
    uint32_t next = active_bus->original_store_multiple(
        cpu, address, mask, direction, cycle_counter);
    trace_reset_stack_multiple(cpu, address, mask, direction);
    return next;
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

static uint32_t supercard_load8(struct ARMCore *cpu, uint32_t address,
                                int *cycle_counter) {
    uint32_t value;
    if (active_bus && supercard_read(active_bus->model, address, 8, &value)) {
        return value;
    }
    return active_bus->original_load8(cpu, address, cycle_counter);
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
    if (active_bus && supercard_store_memory(active_bus->model, address, 8,
                                              (uint8_t)value)) {
        return;
    }
    active_bus->original_store8(cpu, address, value, cycle_counter);
    trace_reset_stack_write(cpu, address, 8, (uint8_t)value);
}

static void supercard_store16(struct ARMCore *cpu, uint32_t address,
                              int16_t value, int *cycle_counter) {
    if (active_bus && address == 0x09FFFFFEu) {
        (void) supercard_write(active_bus->model, address, (uint16_t) value);
        sync_cartridge_fetch_map(active_bus);
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
    if (active_bus && supercard_store_memory(active_bus->model, address, 16,
                                              (uint16_t)value)) {
        return;
    }
    active_bus->original_store16(cpu, address, value, cycle_counter);
    trace_reset_stack_write(cpu, address, 16, (uint16_t)value);
}

static void supercard_store32(struct ARMCore *cpu, uint32_t address,
                              int32_t value, int *cycle_counter) {
    count_sd_write(address);
    if (active_bus && !is_lite_data_address(address) &&
        supercard_store_memory(active_bus->model, address, 32,
                                              (uint32_t)value)) {
        return;
    }
    if (active_bus && supercard_sd_data_write(active_bus->model, address, 32,
                                               (uint32_t)value)) {
        return;
    }
    active_bus->original_store32(cpu, address, value, cycle_counter);
    trace_reset_stack_write(cpu, address, 32, (uint32_t)value);
}

int supercard_mgba_bus_attach(SuperCardMGBABus *bus, struct mCore *core,
                              SuperCardModel *model) {
    if (!bus || !core || !core->cpu || !model || !model->sdram ||
        !model->sdram_size ||
        (model->sdram_size & (model->sdram_size - 1u)) != 0 || active_bus) {
        return 0;
    }
    struct ARMCore *cpu = core->cpu;
    bus->core = core;
    bus->model = model;
    struct GBA *gba = core->board;
    bus->original_rom = gba->memory.rom;
    bus->original_rom_size = gba->memory.romSize;
    bus->original_rom_mask = gba->memory.romMask;
    bus->original_load_multiple = cpu->memory.loadMultiple;
    bus->original_load32 = cpu->memory.load32;
    bus->original_load16 = cpu->memory.load16;
    bus->original_load8 = cpu->memory.load8;
    bus->original_store_multiple = cpu->memory.storeMultiple;
    bus->original_store8 = cpu->memory.store8;
    bus->original_store16 = cpu->memory.store16;
    bus->original_store32 = cpu->memory.store32;
    if (!bus->original_load_multiple || !bus->original_load32 ||
        !bus->original_load16 || !bus->original_load8 ||
        !bus->original_store_multiple || !bus->original_store8 ||
        !bus->original_store16 || !bus->original_store32) {
        bus->core = NULL;
        bus->model = NULL;
        return 0;
    }
    active_bus = bus;
    cpu->memory.loadMultiple = supercard_load_multiple;
    cpu->memory.storeMultiple = supercard_store_multiple;
    cpu->memory.load32 = supercard_load32;
    cpu->memory.load16 = supercard_load16;
    cpu->memory.load8 = supercard_load8;
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
    set_cartridge_fetch_map(bus, bus->original_rom,
                            bus->original_rom_size,
                            bus->original_rom_mask);
    cpu->memory.loadMultiple = bus->original_load_multiple;
    cpu->memory.storeMultiple = bus->original_store_multiple;
    cpu->memory.load32 = bus->original_load32;
    cpu->memory.load16 = bus->original_load16;
    cpu->memory.load8 = bus->original_load8;
    cpu->memory.store8 = bus->original_store8;
    cpu->memory.store16 = bus->original_store16;
    cpu->memory.store32 = bus->original_store32;
    active_bus = NULL;
    bus->core = NULL;
    bus->model = NULL;
    bus->original_rom = NULL;
    bus->original_rom_size = 0;
    bus->original_rom_mask = 0;
    bus->original_load16 = NULL;
    bus->original_load32 = NULL;
    bus->original_load_multiple = NULL;
    bus->original_load8 = NULL;
    bus->original_store_multiple = NULL;
    bus->original_store8 = NULL;
    bus->original_store16 = NULL;
    bus->original_store32 = NULL;
}
