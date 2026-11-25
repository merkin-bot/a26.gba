#include "ezflash_mgba_sd.h"

#include <mgba/core/config.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/gba/core.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/io.h>
#include <mgba/internal/gba/input.h>
#include <mgba-util/image.h>
#include <mgba-util/image.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static void quiet_log(struct mLogger *logger, int category,
                      enum mLogLevel level, const char *format, va_list args) {
    (void)logger; (void)category; (void)level; (void)format; (void)args;
}

int main(int argc, char **argv) {
    if (argc < 4 || argc > 6) {
        fprintf(stderr, "usage: %s <matching-ez-kernel.gba> <frames> <raw-sd-image> [capture.ppm [frame:mask,...]]\n", argv[0]);
        return 2;
    }
    char *end = NULL;
    unsigned long frames = strtoul(argv[2], &end, 10);
    if (!end || *end || !frames || frames > 1000000) return 2;
    mLogSetDefaultLogger(&(struct mLogger){.log = quiet_log});
    struct mCore *core = GBACoreCreate();
    EZFlashSDModel model = {0};
    EZFlashMGBASD adapter = {0};
    mColor *video = NULL;
    int config_ready = 0;
    int result = 1;
    if (!core || !core->init(core)) goto cleanup;
    mCoreInitConfig(core, NULL);
    config_ready = 1;
    if (!mCoreLoadFile(core, argv[1])) {
        fprintf(stderr, "mGBA could not load kernel %s\n", argv[1]);
        goto cleanup;
    }
    if (!ezflash_sd_model_open(&model, argv[3], 0)) {
        fprintf(stderr, "could not open raw SD image %s\n", argv[3]);
        goto cleanup;
    }
    unsigned width = 0, height = 0;
    core->baseVideoSize(core, &width, &height);
    if (!width || !height || !(video = calloc((size_t)width * height, sizeof(*video))))
        goto cleanup;
    core->setVideoBuffer(core, video, width);
    core->reset(core);
    if (!ezflash_mgba_sd_attach(&adapter, core, &model)) goto cleanup;
    unsigned long key_frame[64];
    uint32_t key_mask[64];
    size_t key_count = 0;
    if (argc == 6) {
        char *events = strdup(argv[5]);
        if (!events) goto cleanup;
        char *save = NULL;
        for (char *event = strtok_r(events, ",", &save); event;
             event = strtok_r(NULL, ",", &save)) {
            char *colon = strchr(event, ':');
            char *frame_end = NULL, *mask_end = NULL;
            if (!colon || key_count == 64) { free(events); goto cleanup; }
            *colon++ = '\0';
            unsigned long at = strtoul(event, &frame_end, 10);
            unsigned long mask = strtoul(colon, &mask_end, 0);
            if (!frame_end || *frame_end || !mask_end || *mask_end ||
                at >= frames || mask > 0x3FFu) { free(events); goto cleanup; }
            key_frame[key_count] = at;
            key_mask[key_count++] = (uint32_t)mask;
        }
        free(events);
    }
    uint32_t frame_start = core->frameCounter(core);
    int trace_pc = getenv("A26_EZ_TRACE_PC") != NULL;
    for (unsigned long frame = 0; frame < frames; ++frame) {
        uint32_t keys = 0;
        for (size_t i = 0; i < key_count; ++i)
            if (key_frame[i] == frame) { keys = key_mask[i]; break; }
        core->setKeys(core, keys);
        core->runFrame(core);
        if (trace_pc) {
            uint32_t frame_pc = 0;
            (void)core->readRegister(core, "pc", &frame_pc);
            printf("trace frame=%lu pc=0x%08" PRIX32 " sd_commands=%" PRIu64
                   " sd_reads=%" PRIu64 " sd_writes=%" PRIu64 "\n",
                   frame, frame_pc, model.commands, model.blocks_read,
                   model.blocks_written);
        }
    }
    uint32_t pc = 0;
    (void)core->readRegister(core, "pc", &pc);
    struct GBA *gba = core->board;
    unsigned video_width = 0, video_height = 0;
    const void *pixels = NULL;
    size_t stride = 0;
    core->currentVideoSize(core, &video_width, &video_height);
    core->getPixels(core, &pixels, &stride);
    uint64_t video_hash = UINT64_C(14695981039346656037);
    if (pixels && stride >= video_width) {
        for (unsigned y = 0; y < video_height; ++y) {
            const mColor *row = (const mColor *)pixels + y * stride;
            for (unsigned x = 0; x < video_width; ++x) {
                mColor color = row[x];
                for (unsigned b = 0; b < sizeof(color); ++b) {
                    video_hash ^= (uint8_t)(color >> (b * 8u));
                    video_hash *= UINT64_C(1099511628211);
                }
            }
        }
    }
    if (argc >= 5) {
        FILE *capture = fopen(argv[4], "wb");
        if (!capture || !pixels) {
            if (capture) fclose(capture);
            goto cleanup;
        }
        fprintf(capture, "P6\n%u %u\n255\n", video_width, video_height);
        for (unsigned y = 0; y < video_height; ++y) {
            const mColor *row = (const mColor *)pixels + y * stride;
            for (unsigned x = 0; x < video_width; ++x) {
                mColor color = row[x];
#ifdef COLOR_16_BIT
                uint8_t rgb[3] = {M_R8(color), M_G8(color), M_B8(color)};
#else
                uint8_t rgb[3] = {(uint8_t)color, (uint8_t)(color >> 8),
                                  (uint8_t)(color >> 16)};
#endif
                if (fwrite(rgb, sizeof(rgb), 1, capture) != 1) {
                    fclose(capture);
                    goto cleanup;
                }
            }
        }
        if (fclose(capture)) goto cleanup;
    }
    printf("profile_kernel=%s requested_frames=%lu frame_delta=%" PRIu32
           " pc=0x%08" PRIX32 " commands=%" PRIu64 " blocks_read=%" PRIu64
           " blocks_written=%" PRIu64 " failures=%" PRIu64
           " synthetic_reads=%" PRIu64 " synthetic_writes=%" PRIu64
           " delegated_reads=%" PRIu64 " delegated_writes=%" PRIu64
           " framebuffer=%ux%u fnv64=%016" PRIX64 " capture=%s"
           " ie=0x%04X if=0x%04X ime=0x%04X keycnt=0x%04X keys=0x%03X halted=%d"
           " image=%s status=synthetic-ready-0 timing=UNVERIFIED\n",
           argv[1], frames, core->frameCounter(core) - frame_start, pc,
           model.commands, model.blocks_read, model.blocks_written, model.failures,
           adapter.synthetic_reads, adapter.synthetic_writes,
           adapter.delegated_reads, adapter.delegated_writes,
           video_width, video_height, video_hash, argc >= 5 ? argv[4] : "(none)",
           gba->memory.io[GBA_REG_IE / 2u], gba->memory.io[GBA_REG_IF / 2u],
           gba->memory.io[GBA_REG_IME / 2u], gba->memory.io[GBA_REG_KEYCNT / 2u],
           gba->keysActive, gba->cpu->halted,
           argv[3]);
    result = 0;
cleanup:
    if (adapter.core) ezflash_mgba_sd_detach(&adapter);
    if (model.image) ezflash_sd_model_close(&model);
    free(video);
    if (core) {
        if (config_ready) mCoreConfigDeinit(&core->config);
        core->deinit(core);
    }
    return result;
}
