#ifndef EZFLASH_MGBA_TRACE_H
#define EZFLASH_MGBA_TRACE_H

#include <mgba/internal/arm/arm.h>

#include <stdint.h>

struct mCore;

#define EZFLASH_TRACE_CAPACITY 256u

typedef struct {
    uint32_t address;
    uint32_t value;
    uint8_t width;
    uint8_t is_write;
} EZFlashBusEvent;

typedef struct {
    struct mCore *core;
    uint64_t tracked_reads;
    uint64_t tracked_writes;
    uint64_t dropped_events;
    EZFlashBusEvent events[EZFLASH_TRACE_CAPACITY];
    uint32_t event_count;
    uint32_t total_events;
    uint32_t last_status_value;
    uint32_t status_reads;
    uint32_t data_aperture_reads;
    uint32_t data_aperture_writes;
    uint32_t dma_data_reads;
    uint32_t dma_data_writes;
    uint32_t control_writes;
    uint32_t command_writes;
    uint32_t address_count_writes;
    uint32_t command_end_writes;
    uint32_t save_window_accesses;
    uint32_t psram_nor_window_accesses;

    uint32_t (*original_load32)(struct ARMCore *, uint32_t, int *);
    uint32_t (*original_load16)(struct ARMCore *, uint32_t, int *);
    uint32_t (*original_load8)(struct ARMCore *, uint32_t, int *);
    uint32_t (*original_load_multiple)(struct ARMCore *, uint32_t, int,
                                       enum LSMDirection, int *);
    void (*original_store32)(struct ARMCore *, uint32_t, int32_t, int *);
    void (*original_store16)(struct ARMCore *, uint32_t, int16_t, int *);
    void (*original_store8)(struct ARMCore *, uint32_t, int8_t, int *);
    uint32_t (*original_store_multiple)(struct ARMCore *, uint32_t, int,
                                        enum LSMDirection, int *);
} EZFlashMGBATrace;

/* Passive trace only: every access remains handled by the original mGBA GBA
 * callback. This does not emulate EZ-FLASH responses, storage, timing or maps. */
int ezflash_mgba_trace_attach(EZFlashMGBATrace *trace, struct mCore *core);
void ezflash_mgba_trace_detach(EZFlashMGBATrace *trace);

#endif
