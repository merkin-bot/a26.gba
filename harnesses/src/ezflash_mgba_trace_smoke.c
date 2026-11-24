#include "ezflash_mgba_trace.h"

#include <mgba/core/config.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/gba/core.h>
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
    CHECK(trace.status_reads == 1 && trace.data_aperture_cpu_reads == 1 &&
          trace.data_aperture_cpu_writes == 0,
          "EZ status/data aperture trace is wrong");
    CHECK(trace.events[0].address == 0x09FE0000u &&
          trace.events[0].value == 0xD200u && trace.events[0].is_write &&
          trace.events[7].address == 0x09FC0000u &&
          trace.events[8].address == 0x09400000u &&
          !trace.events[11].is_write,
          "EZ transcript ordering or direction is wrong");

    puts("EZ-FLASH passive GBA bus transcript trace passed; card effects remain unmodeled");
    result = 0;
fail:
    if (trace.core) ezflash_mgba_trace_detach(&trace);
    if (core) {
        if (config_initialized) mCoreConfigDeinit(&core->config);
        core->deinit(core);
    }
    return result;
}
