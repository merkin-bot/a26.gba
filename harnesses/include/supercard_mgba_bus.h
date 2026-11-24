#ifndef SUPERCARD_MGBA_BUS_H
#define SUPERCARD_MGBA_BUS_H

#include "supercard_model.h"
#include <mgba/internal/arm/arm.h>

#include <stdint.h>

struct ARMCore;
struct mCore;

typedef void (*SuperCardStore16)(struct ARMCore *, uint32_t, int16_t, int *);
typedef void (*SuperCardStore8)(struct ARMCore *, uint32_t, int8_t, int *);
typedef void (*SuperCardStore32)(struct ARMCore *, uint32_t, int32_t, int *);
typedef uint32_t (*SuperCardLoad16)(struct ARMCore *, uint32_t, int *);
typedef uint32_t (*SuperCardLoad32)(struct ARMCore *, uint32_t, int *);
typedef uint32_t (*SuperCardLoad8)(struct ARMCore *, uint32_t, int *);
typedef uint32_t (*SuperCardLoadMultiple)(struct ARMCore *, uint32_t, int,
                                        enum LSMDirection, int *);
typedef uint32_t (*SuperCardStoreMultiple)(struct ARMCore *, uint32_t, int,
                                          enum LSMDirection, int *);

#define SUPERCARD_CMD_TRACE_CAPACITY 64u
#define SUPERCARD_RESET_TRACE_CAPACITY 32u

typedef struct {
    uint32_t pc;
    uint32_t address;
    uint32_t value;
    uint8_t width;
} SuperCardResetStackWrite;

typedef struct {
    struct mCore *core;
    SuperCardModel *model;
    SuperCardLoad16 original_load16;
    SuperCardLoad32 original_load32;
    SuperCardLoad8 original_load8;
    SuperCardLoadMultiple original_load_multiple;
    SuperCardStore8 original_store8;
    SuperCardStore16 original_store16;
    SuperCardStore32 original_store32;
    SuperCardStoreMultiple original_store_multiple;
    uint64_t mode_register_writes;
    uint64_t mode_magic_writes;
    uint64_t mode_value_writes;
    uint16_t last_mode_word;
    uint64_t sd_command_writes;
    uint64_t sd_data_writes;
    uint64_t sd_data_reads;
    uint64_t cartridge_writes;
    uint32_t last_cartridge_write;
    uint16_t cmd_write_trace[SUPERCARD_CMD_TRACE_CAPACITY];
    uint32_t cmd_write_trace_count;
    SuperCardResetStackWrite reset_stack_trace[SUPERCARD_RESET_TRACE_CAPACITY];
    uint32_t reset_stack_trace_count;
    uint32_t reset_stack_trace_total;
} SuperCardMGBABus;

/* One attached GBA core per process is supported by this initial adapter. */
int supercard_mgba_bus_attach(SuperCardMGBABus *bus, struct mCore *core,
                              SuperCardModel *model);
void supercard_mgba_bus_detach(SuperCardMGBABus *bus);

#endif
