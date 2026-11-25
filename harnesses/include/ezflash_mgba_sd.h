#ifndef EZFLASH_MGBA_SD_H
#define EZFLASH_MGBA_SD_H

#include "ezflash_sd_model.h"
#include <mgba/internal/arm/arm.h>

struct mCore;

typedef struct {
    struct mCore *core;
    EZFlashSDModel *model;
    uint64_t upstream_reads;
    uint64_t upstream_writes;
    uint64_t delegated_reads;
    uint64_t delegated_writes;
    uint64_t synthetic_reads;
    uint64_t synthetic_writes;
    uint64_t multi_loads;
    uint64_t multi_stores;
    uint32_t (*load32)(struct ARMCore *, uint32_t, int *);
    uint32_t (*load16)(struct ARMCore *, uint32_t, int *);
    uint32_t (*load8)(struct ARMCore *, uint32_t, int *);
    uint32_t (*load_multiple)(struct ARMCore *, uint32_t, int,
                              enum LSMDirection, int *);
    void (*store32)(struct ARMCore *, uint32_t, int32_t, int *);
    void (*store16)(struct ARMCore *, uint32_t, int16_t, int *);
    void (*store8)(struct ARMCore *, uint32_t, int8_t, int *);
    uint32_t (*store_multiple)(struct ARMCore *, uint32_t, int,
                               enum LSMDirection, int *);
} EZFlashMGBASD;

int ezflash_mgba_sd_attach(EZFlashMGBASD *adapter, struct mCore *core,
                           EZFlashSDModel *model);
void ezflash_mgba_sd_detach(EZFlashMGBASD *adapter);

#endif
