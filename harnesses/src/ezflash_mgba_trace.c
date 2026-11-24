#include "ezflash_mgba_trace.h"

#include <mgba/core/core.h>

#include <string.h>

static EZFlashMGBATrace *active_trace;

static int tracked_address(uint32_t address) {
    return address == 0x09FE0000u || address == 0x09FC0000u ||
           address == 0x09400000u || address == 0x09E00000u ||
           address == 0x09600000u || address == 0x09620000u ||
           address == 0x09640000u ||
           (address >= 0x08000000u && address <= 0x08040000u) ||
           (address >= 0x08800000u && address < 0x09800000u) ||
           (address >= 0x0E000000u && address < 0x0E010000u);
}

static void record_event(EZFlashMGBATrace *trace, uint32_t address,
                         uint32_t value, uint8_t width, int is_write) {
    if (!trace || !tracked_address(address)) {
        return;
    }
    ++trace->total_events;
    if (is_write) {
        ++trace->tracked_writes;
        if (address == 0x09E00000u) ++trace->data_aperture_cpu_writes;
        if (address == 0x09400000u) ++trace->control_writes;
        if (address == 0x09600000u || address == 0x09620000u ||
            address == 0x09640000u) ++trace->address_count_writes;
        if (address == 0x09FC0000u) ++trace->command_end_writes;
        if (address == 0x08000000u || address == 0x08020000u ||
            address == 0x08040000u || address == 0x09FE0000u) {
            ++trace->command_writes;
        }
        if (address >= 0x0E000000u && address < 0x0E010000u) {
            ++trace->save_window_accesses;
        }
        if (address >= 0x08800000u && address < 0x09800000u) {
            ++trace->psram_nor_window_accesses;
        }
    } else {
        ++trace->tracked_reads;
        if (address == 0x09E00000u) {
            ++trace->status_reads;
            trace->last_status_value = value;
            ++trace->data_aperture_cpu_reads;
        }
        if (address >= 0x0E000000u && address < 0x0E010000u) {
            ++trace->save_window_accesses;
        }
        if (address >= 0x08800000u && address < 0x09800000u) {
            ++trace->psram_nor_window_accesses;
        }
    }
    if (trace->event_count < EZFLASH_TRACE_CAPACITY) {
        trace->events[trace->event_count++] = (EZFlashBusEvent) {
            .address = address, .value = value, .width = width,
            .is_write = (uint8_t)!!is_write,
        };
    } else {
        ++trace->dropped_events;
    }
}

static uint32_t trace_load8(struct ARMCore *cpu, uint32_t address,
                            int *cycle_counter) {
    uint32_t value = active_trace->original_load8(cpu, address, cycle_counter);
    record_event(active_trace, address, value, 8, 0);
    return value;
}

static uint32_t trace_load16(struct ARMCore *cpu, uint32_t address,
                             int *cycle_counter) {
    uint32_t value = active_trace->original_load16(cpu, address, cycle_counter);
    record_event(active_trace, address, value, 16, 0);
    return value;
}

static uint32_t trace_load32(struct ARMCore *cpu, uint32_t address,
                             int *cycle_counter) {
    uint32_t value = active_trace->original_load32(cpu, address, cycle_counter);
    record_event(active_trace, address, value, 32, 0);
    return value;
}

static uint32_t trace_load_multiple(struct ARMCore *cpu, uint32_t address,
                                    int mask, enum LSMDirection direction,
                                    int *cycle_counter) {
    uint32_t next = active_trace->original_load_multiple(
        cpu, address, mask, direction, cycle_counter);
    int count = 0;
    for (unsigned reg = 0; reg < 16; ++reg) count += (mask >> reg) & 1;
    int offset = (direction & LSM_D) ? -4 : 4;
    if (direction & LSM_D) address -= ((uint32_t)count << 2) - 4u;
    if (direction & LSM_B) address += (uint32_t)offset;
    for (unsigned reg = 0; reg < 16; ++reg) {
        if (!(mask & (1 << reg))) continue;
        record_event(active_trace, address, cpu->gprs[reg], 32, 0);
        address += (uint32_t)offset;
    }
    return next;
}

static void trace_store8(struct ARMCore *cpu, uint32_t address, int8_t value,
                         int *cycle_counter) {
    active_trace->original_store8(cpu, address, value, cycle_counter);
    record_event(active_trace, address, (uint8_t)value, 8, 1);
}

static void trace_store16(struct ARMCore *cpu, uint32_t address, int16_t value,
                          int *cycle_counter) {
    active_trace->original_store16(cpu, address, value, cycle_counter);
    record_event(active_trace, address, (uint16_t)value, 16, 1);
}

static void trace_store32(struct ARMCore *cpu, uint32_t address, int32_t value,
                          int *cycle_counter) {
    active_trace->original_store32(cpu, address, value, cycle_counter);
    record_event(active_trace, address, (uint32_t)value, 32, 1);
}

static uint32_t trace_store_multiple(struct ARMCore *cpu, uint32_t address,
                                     int mask, enum LSMDirection direction,
                                     int *cycle_counter) {
    uint32_t base_address = address;
    int count = 0;
    for (unsigned reg = 0; reg < 16; ++reg) count += (mask >> reg) & 1;
    int offset = (direction & LSM_D) ? -4 : 4;
    if (direction & LSM_D) address -= ((uint32_t)count << 2) - 4u;
    if (direction & LSM_B) address += (uint32_t)offset;
    for (unsigned reg = 0; reg < 16; ++reg) {
        if (!(mask & (1 << reg))) continue;
        uint32_t value = cpu->gprs[reg];
        if (reg == ARM_PC) value += WORD_SIZE_ARM;
        record_event(active_trace, address, value, 32, 1);
        address += (uint32_t)offset;
    }
    uint32_t next = active_trace->original_store_multiple(
        cpu, base_address, mask, direction, cycle_counter);
    return next;
}

int ezflash_mgba_trace_attach(EZFlashMGBATrace *trace, struct mCore *core) {
    if (!trace || !core || !core->cpu || active_trace) return 0;
    struct ARMCore *cpu = core->cpu;
    memset(trace, 0, sizeof(*trace));
    trace->core = core;
    trace->original_load8 = cpu->memory.load8;
    trace->original_load16 = cpu->memory.load16;
    trace->original_load32 = cpu->memory.load32;
    trace->original_load_multiple = cpu->memory.loadMultiple;
    trace->original_store8 = cpu->memory.store8;
    trace->original_store16 = cpu->memory.store16;
    trace->original_store32 = cpu->memory.store32;
    trace->original_store_multiple = cpu->memory.storeMultiple;
    if (!trace->original_load8 || !trace->original_load16 ||
        !trace->original_load32 || !trace->original_load_multiple ||
        !trace->original_store8 || !trace->original_store16 ||
        !trace->original_store32 || !trace->original_store_multiple) {
        memset(trace, 0, sizeof(*trace));
        return 0;
    }
    active_trace = trace;
    cpu->memory.load8 = trace_load8;
    cpu->memory.load16 = trace_load16;
    cpu->memory.load32 = trace_load32;
    cpu->memory.loadMultiple = trace_load_multiple;
    cpu->memory.store8 = trace_store8;
    cpu->memory.store16 = trace_store16;
    cpu->memory.store32 = trace_store32;
    cpu->memory.storeMultiple = trace_store_multiple;
    return 1;
}

void ezflash_mgba_trace_detach(EZFlashMGBATrace *trace) {
    if (!trace || active_trace != trace || !trace->core || !trace->core->cpu) {
        return;
    }
    struct ARMCore *cpu = trace->core->cpu;
    cpu->memory.load8 = trace->original_load8;
    cpu->memory.load16 = trace->original_load16;
    cpu->memory.load32 = trace->original_load32;
    cpu->memory.loadMultiple = trace->original_load_multiple;
    cpu->memory.store8 = trace->original_store8;
    cpu->memory.store16 = trace->original_store16;
    cpu->memory.store32 = trace->original_store32;
    cpu->memory.storeMultiple = trace->original_store_multiple;
    active_trace = NULL;
    memset(trace, 0, sizeof(*trace));
}
