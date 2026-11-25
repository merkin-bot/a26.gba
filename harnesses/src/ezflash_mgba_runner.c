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

typedef struct {
    unsigned long first_frame;
    unsigned long last_frame;
    uint32_t mask;
} KeyEvent;

int main(int argc, char **argv) {
    if (argc < 4 || argc > 6) {
        fprintf(stderr, "usage: %s <matching-ez-kernel.gba> <frames> <raw-sd-image> [capture.ppm [frame:mask,...]]\n", argv[0]);
        fprintf(stderr, "  key events accept taps (300:0x80) or held ranges (300-310:0x80)\n");
        return 2;
    }
    char *end = NULL;
    unsigned long frames = strtoul(argv[2], &end, 10);
    if (!end || *end || !frames || frames > 1000000) return 2;
    mLogSetDefaultLogger(&(struct mLogger){.log = quiet_log});
    struct mCore *core = GBACoreCreate();
    EZFlashSDModel model = {0};
    EZFlashPSRAMModel psram = {0};
    EZFlashNORModel nor = {0};
    EZFlashNORModel nor_s98 = {0};
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
    if (!ezflash_psram_model_init(&psram)) {
        fputs("could not allocate synthetic EZ PSRAM\n", stderr);
        goto cleanup;
    }
    if (!ezflash_nor_model_init(&nor, argv[1])) {
        fputs("could not initialize source-shaped synthetic S71 NOR\n", stderr);
        goto cleanup;
    }
    if (!ezflash_nor_model_init_window(&nor_s98, EZFLASH_S98_NOR_BASE,
                                       0x20000u, NULL, 0xFFFFu)) {
        fputs("could not initialize source-shaped synthetic S98 NOR\n", stderr);
        goto cleanup;
    }
    unsigned width = 0, height = 0;
    core->baseVideoSize(core, &width, &height);
    if (!width || !height || !(video = calloc((size_t)width * height, sizeof(*video))))
        goto cleanup;
    core->setVideoBuffer(core, video, width);
    core->reset(core);
    if (!ezflash_mgba_sd_attach(&adapter, core, &model, &psram, &nor,
                                &nor_s98)) goto cleanup;
    KeyEvent key_events[64];
    size_t key_count = 0;
    if (argc == 6) {
        char *events = strdup(argv[5]);
        if (!events) goto cleanup;
        char *save = NULL;
        for (char *event = strtok_r(events, ",", &save); event;
             event = strtok_r(NULL, ",", &save)) {
            char *colon = strchr(event, ':');
            char *frame_end = NULL, *last_end = NULL, *mask_end = NULL;
            if (!colon || key_count == 64) { free(events); goto cleanup; }
            *colon = '\0';
            char *mask_text = colon + 1;
            char *dash = strchr(event, '-');
            unsigned long first, last;
            if (dash) {
                *dash = '\0';
                if (!event[0] || !dash[1]) { free(events); goto cleanup; }
                first = strtoul(event, &frame_end, 10);
                last = strtoul(dash + 1, &last_end, 10);
            } else {
                first = strtoul(event, &frame_end, 10);
                last = first;
            }
            unsigned long mask = strtoul(mask_text, &mask_end, 0);
            if (!frame_end || frame_end == event || *frame_end ||
                !mask_end || mask_end == mask_text || *mask_end ||
                (dash && (!last_end || last_end == dash + 1 || *last_end)) ||
                first > last || last >= frames || mask > 0x3FFu) {
                free(events);
                goto cleanup;
            }
            key_events[key_count++] = (KeyEvent){first, last, (uint32_t)mask};
        }
        free(events);
    }
    uint32_t frame_start = core->frameCounter(core);
    int trace_pc = getenv("A26_EZ_TRACE_PC") != NULL;
    for (unsigned long frame = 0; frame < frames; ++frame) {
        uint32_t keys = 0;
        for (size_t i = 0; i < key_count; ++i)
            if (frame >= key_events[i].first_frame &&
                frame <= key_events[i].last_frame)
                keys |= key_events[i].mask;
        core->setKeys(core, keys);
        core->runFrame(core);
        if (trace_pc) {
            uint32_t frame_pc = 0;
            (void)core->readRegister(core, "pc", &frame_pc);
            printf("trace frame=%lu pc=0x%08" PRIX32 " sd_commands=%" PRIu64
                   " sd_reads=%" PRIu64 " sd_writes=%" PRIu64
                   " last_lba=%" PRIu32 " last_blocks=%u last_write=%u"
                   " keys=0x%03X keycnt=0x%04X ie=0x%04X if=0x%04X"
                   " ime=0x%04X dispstat=0x%04X vcount=%u"
                   " irq_masked=%u halted=%d\n",
                   frame, frame_pc, model.commands, model.blocks_read,
                   model.blocks_written, model.last_command_sector,
                   model.last_command_blocks, model.last_command_write,
                   ((struct GBA *)core->board)->keysActive,
                   ((struct GBA *)core->board)->memory.io[GBA_REG_KEYCNT / 2u],
                   ((struct GBA *)core->board)->memory.io[GBA_REG_IE / 2u],
                   ((struct GBA *)core->board)->memory.io[GBA_REG_IF / 2u],
                   ((struct GBA *)core->board)->memory.io[GBA_REG_IME / 2u],
                   ((struct GBA *)core->board)->memory.io[GBA_REG_DISPSTAT / 2u],
                   ((struct GBA *)core->board)->memory.io[GBA_REG_VCOUNT / 2u],
                   ((struct GBA *)core->board)->cpu->cpsr.i,
                   ((struct GBA *)core->board)->cpu->halted);
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
           " psram_reads=%" PRIu64 " psram_writes=%" PRIu64
           " nor_reads=%" PRIu64 " nor_writes=%" PRIu64
           " nor_s98_reads=%" PRIu64 " nor_s98_writes=%" PRIu64
           " nor_erases=%" PRIu64 " nor_program_words=%" PRIu64
           " nor_s98_erases=%" PRIu64 " nor_s98_program_words=%" PRIu64
           " delegated_reads=%" PRIu64 " delegated_writes=%" PRIu64
           " framebuffer=%ux%u fnv64=%016" PRIX64 " capture=%s"
           " ie=0x%04X if=0x%04X ime=0x%04X dispstat=0x%04X vcount=%u"
           " keycnt=0x%04X keys=0x%03X halted=%d"
           " image=%s status=synthetic-ready-0 timing=UNVERIFIED\n",
           argv[1], frames, core->frameCounter(core) - frame_start, pc,
           model.commands, model.blocks_read, model.blocks_written, model.failures,
           adapter.synthetic_reads, adapter.synthetic_writes,
           adapter.psram_reads, adapter.psram_writes,
           adapter.nor_reads, adapter.nor_writes,
           adapter.nor_s98_reads, adapter.nor_s98_writes,
           nor.erase_commands, nor.program_words,
           nor_s98.erase_commands, nor_s98.program_words,
           adapter.delegated_reads, adapter.delegated_writes,
           video_width, video_height, video_hash, argc >= 5 ? argv[4] : "(none)",
           gba->memory.io[GBA_REG_IE / 2u], gba->memory.io[GBA_REG_IF / 2u],
           gba->memory.io[GBA_REG_IME / 2u], gba->memory.io[GBA_REG_DISPSTAT / 2u],
           gba->memory.io[GBA_REG_VCOUNT / 2u], gba->memory.io[GBA_REG_KEYCNT / 2u],
           gba->keysActive, gba->cpu->halted,
           argv[3]);
    if (getenv("A26_EZ_TRACE_REGS")) {
        fprintf(stderr, "final_cpu cpsr=0x%08" PRIX32, gba->cpu->cpsr.packed);
        for (unsigned reg = 0; reg < 16; ++reg)
            fprintf(stderr, " r%u=0x%08" PRIX32, reg, gba->cpu->gprs[reg]);
        fputc('\n', stderr);
    }
    const char *trace_word = getenv("A26_EZ_TRACE_EWRAM");
    if (trace_word) {
        char *address_end = NULL;
        unsigned long address = strtoul(trace_word, &address_end, 0);
        if (!address_end || *address_end || address < 0x02000000u ||
            address > 0x02040000u - 8u || (address & 3u)) {
            fprintf(stderr, "invalid A26_EZ_TRACE_EWRAM address\n");
            goto cleanup;
        }
        size_t offset = (address - 0x02000000u) / 4u;
        printf("ewram[%08lX]=%08" PRIX32 " ewram[%08lX]=%08" PRIX32 "\n",
               address, gba->memory.wram[offset], address + 4u,
               gba->memory.wram[offset + 1u]);
    }
    result = 0;
cleanup:
    if (adapter.core) ezflash_mgba_sd_detach(&adapter);
    if (model.image) ezflash_sd_model_close(&model);
    ezflash_psram_model_deinit(&psram);
    ezflash_nor_model_deinit(&nor);
    ezflash_nor_model_deinit(&nor_s98);
    free(video);
    if (core) {
        if (config_ready) mCoreConfigDeinit(&core->config);
        core->deinit(core);
    }
    return result;
}
