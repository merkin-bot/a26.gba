#include "ezflash_mgba_trace.h"

#include <mgba/core/config.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/gba/core.h>
#include <mgba/internal/gba/dma.h>
#include <mgba/internal/gba/gba.h>

#include <stdarg.h>
#include <stdio.h>

static void quiet_log(struct mLogger *logger, int category,
                      enum mLogLevel level, const char *format, va_list args) {
    (void)logger; (void)category; (void)level; (void)format; (void)args;
}

static struct mLogger silent_logger = {.log = quiet_log, .filter = NULL};

#define CHECK(condition, message) do { \
    if (!(condition)) { fputs(message "\n", stderr); goto fail; } \
} while (0)

static int run_dma_probe(int attach_trace, int card_to_ram, uint32_t *value,
                         uint32_t *dma_when, int32_t *dma_seq_cycles,
                         uint32_t *dma_reads, uint32_t *dma_writes,
                         uint32_t *aperture_reads, uint32_t *aperture_writes,
                         uint32_t *aperture_write_value,
                         EZFlashMGBATrace *trace) {
    struct mCore *core = GBACoreCreate();
    if (!core || !core->init(core)) return 0;
    mCoreInitConfig(core, NULL);
    core->reset(core);
    struct GBA *gba = core->board;
    if (attach_trace && !ezflash_mgba_trace_attach(trace, core)) {
        mCoreConfigDeinit(&core->config);
        core->deinit(core);
        return 0;
    }

    static const uint32_t payload[4] = {
        0x5A69B478u, 0x11223344u, 0x89ABCDEFu, 0xDEADBEEFu,
    };
    const uint32_t source = card_to_ram ? 0x09E00000u : 0x02000008u;
    const uint32_t dest = card_to_ram ? 0x02000004u : 0x09E00000u;
    if (!card_to_ram) {
        for (unsigned i = 0; i < 4; ++i)
            core->busWrite32(core, source + i * 4u, payload[i]);
    }

    /* DMA3 is the GBA channel that preserves the full cartridge source range. */
    core->busWrite32(core, 0x040000D4u, source);
    core->busWrite32(core, 0x040000D8u, dest);
    core->busWrite16(core, 0x040000DCu, 4u);
    core->busWrite16(core, 0x040000DEu, 0x8400u);
    if (gba->memory.dma[3].nextCount != 4) return 0;

    /* Give the CPU a stable ARM branch loop so its normal run loop advances
     * mGBA's scheduled DMA event rather than stopping in the reset BIOS state. */
    core->busWrite32(core, 0x03000000u, 0xEAFFFFFEu);
    struct ARMCore *cpu = core->cpu;
    cpu->executionMode = MODE_ARM;
    cpu->cpsr.t = 0;
    cpu->gprs[ARM_PC] = 0x03000000u;
    cpu->memory.setActiveRegion(cpu, 0x03000000u);
    cpu->prefetch[0] = 0xEAFFFFFEu;
    cpu->prefetch[1] = 0xEAFFFFFEu;
    cpu->gprs[ARM_PC] = 0x03000008u;
    cpu->cycles = 0;
    cpu->nextEvent = 3;
    for (unsigned i = 0; i < 600 && gba->memory.dma[3].nextCount > 0 &&
                            !(gba->memory.dma[3].nextCount & 0x80000000); ++i) {
        core->runLoop(core);
    }
    uint32_t hash = UINT32_C(2166136261);
    for (unsigned i = 0; i < 4; ++i) {
        uint32_t word = card_to_ram ? core->busRead32(core, dest + i * 4u) :
                                     payload[i];
        if (!card_to_ram && attach_trace) {
            for (uint32_t event = 0; event < trace->event_count; ++event) {
                if (trace->events[event].address == 0x09E00000u + i * 4u &&
                    trace->events[event].is_write) {
                    word = trace->events[event].value;
                    break;
                }
            }
        }
        hash = (hash ^ word) * UINT32_C(16777619);
    }
    *value = hash;
    *dma_when = gba->memory.dma[3].when;
    *dma_seq_cycles = gba->memory.dma[3].cycles;
    *dma_reads = trace->dma_data_reads;
    *dma_writes = trace->dma_data_writes;
    *aperture_reads = trace->data_aperture_reads;
    *aperture_writes = trace->data_aperture_writes;
    *aperture_write_value = 0;
    for (uint32_t i = 0; i < trace->event_count; ++i) {
        if (trace->events[i].address >= 0x09E00000u &&
            trace->events[i].address < 0x09F00000u &&
            trace->events[i].is_write) {
            *aperture_write_value = trace->events[i].value;
        }
    }

    if (trace->core) ezflash_mgba_trace_detach(trace);
    mCoreConfigDeinit(&core->config);
    core->deinit(core);
    return 1;
}

int main(void) {
    mLogSetDefaultLogger(&silent_logger);
    struct mCore *core = GBACoreCreate();
    EZFlashMGBATrace trace = {0};
    int config_initialized = 0;
    int result = 1;
    CHECK(core && core->init(core), "mGBA GBA core initialization failed");
    mCoreInitConfig(core, NULL);
    config_initialized = 1;
    core->reset(core);
    struct ARMCore *cpu = core->cpu;
    uint32_t (*base_load16)(struct ARMCore *, uint32_t, int *) = cpu->memory.load16;
    uint32_t (*base_load32)(struct ARMCore *, uint32_t, int *) = cpu->memory.load32;
    void (*base_store16)(struct ARMCore *, uint32_t, int16_t, int *) = cpu->memory.store16;
    void (*base_store32)(struct ARMCore *, uint32_t, int32_t, int *) = cpu->memory.store32;
    CHECK(ezflash_mgba_trace_attach(&trace, core),
          "EZ-FLASH passive mGBA trace attach failed");

    core->busWrite16(core, 0x09FE0000u, 0xD200u);
    core->busWrite16(core, 0x08000000u, 0x1500u);
    core->busWrite16(core, 0x08020000u, 0xD200u);
    core->busWrite16(core, 0x08040000u, 0x1500u);
    core->busWrite16(core, 0x09600000u, 0x1234u);
    core->busWrite16(core, 0x09620000u, 0x0002u);
    core->busWrite16(core, 0x09640000u, 0x0004u);
    core->busWrite16(core, 0x09FC0000u, 0x1500u);
    core->busWrite16(core, 0x09400000u, 0x0001u);
    core->busWrite16(core, 0x09400000u, 0x0003u);
    core->busWrite16(core, 0x09400000u, 0x0000u);
    (void)core->busRead16(core, 0x09E00000u);

    CHECK(trace.event_count == 12 && trace.total_events == 12,
          "unexpected EZ transcript event count");
    CHECK(trace.command_writes == 4 && trace.address_count_writes == 3 &&
          trace.command_end_writes == 1 && trace.control_writes == 3,
          "EZ command transcript counters are wrong");
    CHECK(trace.status_reads == 1 && trace.data_aperture_reads == 1 &&
          trace.data_aperture_writes == 0,
          "EZ status/data aperture trace is wrong");
    CHECK(trace.events[0].address == 0x09FE0000u &&
          trace.events[0].value == 0xD200u && trace.events[0].is_write &&
          trace.events[7].address == 0x09FC0000u &&
          trace.events[8].address == 0x09400000u &&
          !trace.events[11].is_write,
          "EZ transcript ordering or direction is wrong");

    ezflash_mgba_trace_detach(&trace);
    CHECK(cpu->memory.load16 == base_load16 && cpu->memory.load32 == base_load32 &&
          cpu->memory.store16 == base_store16 && cpu->memory.store32 == base_store32,
          "EZ passive trace detach did not restore GBA callbacks");

    EZFlashMGBATrace dma_trace = {0};
    uint32_t baseline_read, traced_read, baseline_write, traced_write;
    uint32_t dma_reads, dma_writes;
    uint32_t baseline_when, traced_when;
    int32_t baseline_seq, traced_seq;
    uint32_t aperture_reads, aperture_writes, aperture_write_value;
    CHECK(run_dma_probe(0, 1, &baseline_read, &baseline_when, &baseline_seq,
                        &dma_reads, &dma_writes, &aperture_reads,
                        &aperture_writes, &aperture_write_value, &dma_trace),
          "baseline card-to-EWRAM mGBA DMA probe failed");
    CHECK(run_dma_probe(1, 1, &traced_read, &traced_when, &traced_seq,
                        &dma_reads, &dma_writes, &aperture_reads,
                        &aperture_writes, &aperture_write_value, &dma_trace),
          "traced card-to-EWRAM mGBA DMA probe failed");
    CHECK(traced_read == baseline_read && traced_when == baseline_when &&
          traced_seq == baseline_seq,
          "passive trace changed DMA values or GBA timing");
    CHECK(dma_reads == 4 && aperture_reads == 4 && aperture_writes == 0,
          "mGBA DMA read did not reach EZ aperture trace callback");
    CHECK(run_dma_probe(0, 0, &baseline_write, &baseline_when, &baseline_seq,
                        &dma_reads, &dma_writes, &aperture_reads,
                        &aperture_writes, &aperture_write_value, &dma_trace),
          "baseline EWRAM-to-card mGBA DMA probe failed");
    CHECK(run_dma_probe(1, 0, &traced_write, &traced_when, &traced_seq,
                        &dma_reads, &dma_writes, &aperture_reads,
                        &aperture_writes, &aperture_write_value, &dma_trace),
          "traced EWRAM-to-card mGBA DMA probe failed");
    CHECK(traced_write == baseline_write && traced_when == baseline_when &&
          traced_seq == baseline_seq,
          "passive trace changed EWRAM-to-card DMA data or timing");
    CHECK(dma_reads == 0 && dma_writes == 4 && aperture_reads == 0 &&
          aperture_writes == 4 && aperture_write_value == 0xDEADBEEFu,
          "mGBA DMA write did not reach EZ aperture trace callback");

    puts("EZ-FLASH passive transcript and DMA trace passed; upstream DMA values/cycles unchanged; card effects remain unmodeled");
    result = 0;
fail:
    if (trace.core) ezflash_mgba_trace_detach(&trace);
    if (core) {
        if (config_initialized) mCoreConfigDeinit(&core->config);
        core->deinit(core);
    }
    return result;
}
