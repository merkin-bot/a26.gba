#ifndef SUPERCARD_MGBA_BUS_H
#define SUPERCARD_MGBA_BUS_H

#include "supercard_model.h"

struct ARMCore;
struct mCore;

typedef void (*SuperCardStore16)(struct ARMCore *, uint32_t, int16_t, int *);

typedef struct {
    struct mCore *core;
    SuperCardModel *model;
    SuperCardStore16 original_store16;
} SuperCardMGBABus;

/* One attached GBA core per process is supported by this initial adapter. */
int supercard_mgba_bus_attach(SuperCardMGBABus *bus, struct mCore *core,
                              SuperCardModel *model);
void supercard_mgba_bus_detach(SuperCardMGBABus *bus);

#endif
