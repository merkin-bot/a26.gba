#include "supercard_mgba_bus.h"

#include <mgba/core/core.h>
#include <mgba/internal/arm/arm.h>

static SuperCardMGBABus *active_bus;

static void supercard_store16(struct ARMCore *cpu, uint32_t address,
                              int16_t value, int *cycle_counter) {
    if (active_bus && address == 0x09FFFFFEu) {
        (void) supercard_write(active_bus->model, address, (uint16_t) value);
        return;
    }
    active_bus->original_store16(cpu, address, value, cycle_counter);
}

int supercard_mgba_bus_attach(SuperCardMGBABus *bus, struct mCore *core,
                              SuperCardModel *model) {
    if (!bus || !core || !core->cpu || !model || active_bus) {
        return 0;
    }
    struct ARMCore *cpu = core->cpu;
    bus->core = core;
    bus->model = model;
    bus->original_store16 = cpu->memory.store16;
    if (!bus->original_store16) {
        bus->core = NULL;
        bus->model = NULL;
        return 0;
    }
    active_bus = bus;
    cpu->memory.store16 = supercard_store16;
    return 1;
}

void supercard_mgba_bus_detach(SuperCardMGBABus *bus) {
    if (!bus || active_bus != bus || !bus->core || !bus->core->cpu) {
        return;
    }
    struct ARMCore *cpu = bus->core->cpu;
    cpu->memory.store16 = bus->original_store16;
    active_bus = NULL;
    bus->core = NULL;
    bus->model = NULL;
    bus->original_store16 = NULL;
}
