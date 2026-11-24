#include "supercard_mgba_bus.h"

#include <mgba/core/config.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/gba/core.h>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static void quiet_log(struct mLogger *logger, int category,
                      enum mLogLevel level, const char *format, va_list args) {
    (void) logger;
    (void) category;
    (void) level;
    (void) format;
    (void) args;
}

static struct mLogger silent_logger = {.log = quiet_log, .filter = NULL};

static void cleanup(struct mCore *core, SuperCardMGBABus *bus,
                    SuperCardModel *model, int config_initialized) {
    if (bus->core) {
        supercard_mgba_bus_detach(bus);
    }
    if (core) {
        if (config_initialized) {
            mCoreConfigDeinit(&core->config);
        }
        core->deinit(core);
    }
    free(model->cart);
    free(model->sdram);
}

int main(int argc, char **argv) {
    mLogSetDefaultLogger(&silent_logger);
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s <gba-image> [frames]\n", argv[0]);
        return 2;
    }
    char *end = NULL;
    unsigned long frames = argc == 3 ? strtoul(argv[2], &end, 10) : 300;
    if ((argc == 3 && (errno || !end || *end)) || frames == 0 || frames > 1000000) {
        fputs("frame count must be in 1..1000000\n", stderr);
        return 2;
    }

    SuperCardModel model = {0};
    supercard_reset(&model);

    struct mCore *core = GBACoreCreate();
    SuperCardMGBABus bus = {0};
    int config_initialized = 0;
    if (!core || !core->init(core)) {
        fputs("mGBA GBA core initialization failed\n", stderr);
        if (core) {
            core->deinit(core);
        }
        return 1;
    }
    mCoreInitConfig(core, NULL);
    config_initialized = 1;

    if (!supercard_mgba_bus_attach(&bus, core, &model)) {
        fputs("could not attach SuperCard memory hooks\n", stderr);
        cleanup(core, &bus, &model, config_initialized);
        return 1;
    }
    if (!mCoreLoadFile(core, argv[1])) {
        fprintf(stderr, "mGBA could not load %s\n", argv[1]);
        cleanup(core, &bus, &model, config_initialized);
        return 1;
    }

    core->reset(core);
    uint32_t start_frame = core->frameCounter(core);
    for (unsigned long i = 0; i < frames; ++i) {
        core->runFrame(core);
    }
    uint32_t end_frame = core->frameCounter(core);
    printf("mGBA platform=GBA requested_frames=%lu frame_counter_delta=%" PRIu32
           " mode_register_writes=%" PRIu64 " magic_writes=%" PRIu64
           " mode_words=%" PRIu64 " last_mode=0x%04X sd_command_writes=%" PRIu64
           " sd_data_writes=%" PRIu64 " map=%s sd=%d write=%d\n",
           frames, end_frame - start_frame, bus.mode_register_writes,
           bus.mode_magic_writes, bus.mode_value_writes, bus.last_mode_word,
           bus.sd_command_writes, bus.sd_data_writes,
           model.mode == SUPERCARD_MODE_SDRAM ? "sdram" : "firmware",
           model.sd_interface, model.write_access);

    cleanup(core, &bus, &model, config_initialized);
    return 0;
}
