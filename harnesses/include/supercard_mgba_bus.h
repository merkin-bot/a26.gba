#ifndef SUPERCARD_MGBA_BUS_H
#define SUPERCARD_MGBA_BUS_H

#include "supercard_model.h"

#include <stdint.h>

struct ARMCore;
struct mCore;

typedef void (*SuperCardStore16)(struct ARMCore *, uint32_t, int16_t, int *);
typedef void (*SuperCardStore8)(struct ARMCore *, uint32_t, int8_t, int *);
typedef void (*SuperCardStore32)(struct ARMCore *, uint32_t, int32_t, int *);

typedef struct {
    struct mCore *core;
    SuperCardModel *model;
    SuperCardStore8 original_store8;
    SuperCardStore16 original_store16;
    SuperCardStore32 original_store32;
    uint64_t mode_register_writes;
    uint64_t mode_magic_writes;
    uint64_t mode_value_writes;
    uint16_t last_mode_word;
    uint64_t sd_command_writes;
    uint64_t sd_data_writes;
} SuperCardMGBABus;

/* One attached GBA core per process is supported by this initial adapter. */
int supercard_mgba_bus_attach(SuperCardMGBABus *bus, struct mCore *core,
                              SuperCardModel *model);
void supercard_mgba_bus_detach(SuperCardMGBABus *bus);

#endif
