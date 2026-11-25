#include "supercard_mgba_bus.h"

#include <mgba/core/config.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/core/timing.h>
#include <mgba/gba/core.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/io.h>
#include <mgba/internal/gba/input.h>
#include <mgba-util/audio-buffer.h>
#include <mgba-util/image.h>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

typedef struct SuperCardKeyEvent {
    unsigned long first_frame;
    unsigned long last_frame;
    uint32_t mask;
} SuperCardKeyEvent;

static void quiet_log(struct mLogger *logger, int category,
                      enum mLogLevel level, const char *format, va_list args) {
    (void) logger;
    (void) category;
    (void) level;
    (void) format;
    (void) args;
}

static struct mLogger silent_logger = {.log = quiet_log, .filter = NULL};

static const char *supercard_board_name(SuperCardBoard board) {
    switch (board) {
    case SUPERCARD_BOARD_SD: return "sd";
    case SUPERCARD_BOARD_CHIS: return "chis";
    case SUPERCARD_BOARD_LITE:
    default: return "lite";
    }
}

static int parse_supercard_board(const char *name, SuperCardBoard *board) {
    if (!name || strcmp(name, "lite") == 0) {
        *board = SUPERCARD_BOARD_LITE;
        return 1;
    }
    if (strcmp(name, "sd") == 0) {
        *board = SUPERCARD_BOARD_SD;
        return 1;
    }
    if (strcmp(name, "chis") == 0) {
        *board = SUPERCARD_BOARD_CHIS;
        return 1;
    }
    return 0;
}

static const char *pc_region(uint32_t pc) {
    if (pc < 0x00004000u) return "bios";
    if (pc >= 0x02000000u && pc < 0x03000000u) return "ewram";
    if (pc >= 0x03000000u && pc < 0x04000000u) return "iwram";
    if (pc >= 0x08000000u && pc < 0x0E000000u) return "cartridge";
    return "other";
}

static uint32_t read_le32(const uint8_t *memory, unsigned offset) {
    return (uint32_t)memory[offset] |
           ((uint32_t)memory[offset + 1u] << 8) |
           ((uint32_t)memory[offset + 2u] << 16) |
           ((uint32_t)memory[offset + 3u] << 24);
}

static unsigned env_offset(const char *name, unsigned fallback) {
    const char *text = getenv(name);
    if (!text || !*text) {
        return fallback;
    }
    char *end = NULL;
    unsigned long value = strtoul(text, &end, 0);
    return end && !*end && value <= 0x7FFCu ? (unsigned)value : fallback;
}

static int capture_frame(struct mCore *core, const char *path,
                         uint64_t *hash_out, unsigned *width_out,
                         unsigned *height_out) {
    unsigned width = 0, height = 0;
    const void *pixels = NULL;
    size_t stride = 0;
    core->currentVideoSize(core, &width, &height);
    core->getPixels(core, &pixels, &stride);
    if (!pixels || !width || !height || stride < width) {
        return 0;
    }
    uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned y = 0; y < height; ++y) {
        const mColor *row = (const mColor *)pixels + y * stride;
        for (unsigned x = 0; x < width; ++x) {
            mColor color = row[x];
            for (unsigned byte = 0; byte < sizeof(color); ++byte) {
                hash ^= (uint8_t)(color >> (byte * 8u));
                hash *= UINT64_C(1099511628211);
            }
        }
    }
    if (path) {
        FILE *file = fopen(path, "wb");
        if (!file) {
            return 0;
        }
        fprintf(file, "P6\n%u %u\n255\n", width, height);
        for (unsigned y = 0; y < height; ++y) {
            const mColor *row = (const mColor *)pixels + y * stride;
            for (unsigned x = 0; x < width; ++x) {
                mColor color = row[x];
#ifdef COLOR_16_BIT
                uint8_t rgb[3] = {M_R8(color), M_G8(color), M_B8(color)};
#else
                uint8_t rgb[3] = {(uint8_t)color, (uint8_t)(color >> 8),
                                   (uint8_t)(color >> 16)};
#endif
                if (fwrite(rgb, sizeof(rgb), 1, file) != 1) {
                    fclose(file);
                    return 0;
                }
            }
        }
        if (fclose(file) != 0) {
            return 0;
        }
    }
    *hash_out = hash;
    *width_out = width;
    *height_out = height;
    return 1;
}

static void put_le16(uint8_t *out, uint16_t value) {
    out[0] = value & 0xFFu;
    out[1] = value >> 8;
}

static void put_le32(uint8_t *out, uint32_t value) {
    out[0] = value & 0xFFu;
    out[1] = (value >> 8) & 0xFFu;
    out[2] = (value >> 16) & 0xFFu;
    out[3] = value >> 24;
}

typedef struct AudioWavCapture {
    FILE *file;
    const char *path;
    unsigned channels;
    unsigned sample_rate;
    uint64_t frames;
    uint64_t sum_squares;
    uint32_t peak;
    uint32_t data_bytes;
} AudioWavCapture;

static int audio_capture_begin(struct mCore *core, const char *path,
                               AudioWavCapture *capture) {
    struct mAudioBuffer *audio = core->getAudioBuffer(core);
    capture->path = path;
    capture->channels = audio->channels;
    capture->sample_rate = core->audioSampleRate(core);
    if (!capture->channels || capture->channels > UINT16_MAX ||
        !capture->sample_rate) {
        fputs("invalid mGBA audio buffer metadata\n", stderr);
        return 0;
    }
    capture->file = fopen(path, "wb+");
    if (!capture->file) {
        fprintf(stderr, "could not open WAV capture %s\n", path);
        return 0;
    }
    uint8_t header[44] = {0};
    memcpy(header, "RIFF", 4);
    memcpy(header + 8, "WAVEfmt ", 8);
    put_le32(header + 16, 16);
    put_le16(header + 20, 1);
    put_le16(header + 22, (uint16_t)capture->channels);
    put_le32(header + 24, capture->sample_rate);
    put_le32(header + 28, capture->sample_rate * capture->channels * 2u);
    put_le16(header + 32, (uint16_t)(capture->channels * 2u));
    put_le16(header + 34, 16);
    memcpy(header + 36, "data", 4);
    if (fwrite(header, sizeof(header), 1, capture->file) != 1) {
        fclose(capture->file);
        capture->file = NULL;
        fprintf(stderr, "could not initialize WAV capture %s\n", path);
        return 0;
    }
    return 1;
}

static int audio_capture_drain(struct mCore *core, AudioWavCapture *capture) {
    struct mAudioBuffer *audio = core->getAudioBuffer(core);
    int16_t samples[4096];
    size_t frames_available = mAudioBufferAvailable(audio);
    while (frames_available) {
        size_t chunk = frames_available > 2048 ? 2048 : frames_available;
        size_t frames = mAudioBufferRead(audio, samples, chunk);
        size_t sample_count = frames * capture->channels;
        size_t bytes = sample_count * sizeof(*samples);
        if (capture->data_bytes > UINT32_MAX - bytes ||
            fwrite(samples, sizeof(*samples), sample_count, capture->file) !=
                sample_count) {
            fprintf(stderr, "WAV capture %s exceeded RIFF limits or write failed\n",
                    capture->path);
            return 0;
        }
        for (size_t i = 0; i < sample_count; ++i) {
            int32_t sample = samples[i];
            int32_t magnitude = sample < 0 ? -sample : sample;
            if ((uint32_t)magnitude > capture->peak) {
                capture->peak = (uint32_t)magnitude;
            }
            capture->sum_squares += (uint64_t)(sample * sample);
        }
        capture->frames += frames;
        capture->data_bytes += (uint32_t)bytes;
        frames_available -= frames;
    }
    return 1;
}

static int audio_capture_finish(struct mCore *core, AudioWavCapture *capture) {
    struct GBA *gba = core->board;
    uint8_t header[44] = {0};
    memcpy(header, "RIFF", 4);
    put_le32(header + 4, 36u + capture->data_bytes);
    memcpy(header + 8, "WAVEfmt ", 8);
    put_le32(header + 16, 16);
    put_le16(header + 20, 1);
    put_le16(header + 22, (uint16_t)capture->channels);
    put_le32(header + 24, capture->sample_rate);
    put_le32(header + 28, capture->sample_rate * capture->channels * 2u);
    put_le16(header + 32, (uint16_t)(capture->channels * 2u));
    put_le16(header + 34, 16);
    memcpy(header + 36, "data", 4);
    put_le32(header + 40, capture->data_bytes);
    int ok = fseek(capture->file, 0, SEEK_SET) == 0 &&
             fwrite(header, sizeof(header), 1, capture->file) == 1;
    if (fclose(capture->file) != 0) ok = 0;
    capture->file = NULL;
    if (!ok) {
        fprintf(stderr, "could not finalize WAV capture %s\n", capture->path);
        return 0;
    }
    uint64_t rms_squared = capture->frames && capture->channels ?
        capture->sum_squares / (capture->frames * capture->channels) : 0;
    printf("audio_state enabled=%u soundcnt_h=0x%04X soundcnt_x=0x%04X"
           " timer0_reload=0x%04X timer0_control=0x%04X"
           " dma1_source=0x%08" PRIX32 " dma1_next_source=0x%08" PRIX32
           " dma1_control=0x%04X fifo_a_read=%d fifo_a_write=%d"
           " fifo_a_remaining=%d left=%u right=%u\n",
           gba->audio.enable,
           gba->memory.io[GBA_REG_SOUNDCNT_HI >> 1],
           gba->memory.io[GBA_REG_SOUNDCNT_X >> 1],
           gba->memory.io[GBA_REG_TM0CNT_LO >> 1],
           gba->memory.io[GBA_REG_TM0CNT_HI >> 1],
           gba->memory.dma[1].source, gba->memory.dma[1].nextSource,
           gba->memory.dma[1].reg, gba->audio.chA.fifoRead,
           gba->audio.chA.fifoWrite, gba->audio.chA.internalRemaining,
           gba->audio.chALeft, gba->audio.chARight);
    printf("audio_capture=%s frames=%" PRIu64 " sample_rate=%u channels=%u"
           " peak=%" PRIu32 " rms_squared=%" PRIu64 "\n",
           capture->path, capture->frames, capture->sample_rate,
           capture->channels, capture->peak, rms_squared);
    return 1;
}

static int stage_kernel_in_sdram(const char *path, SuperCardModel *model) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return 0;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }
    long length = ftell(file);
    if (length <= 0 || (uint64_t)length > model->sdram_size ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    size_t read_count = fread(model->sdram, 1, (size_t)length, file);
    int close_result = fclose(file);
    return read_count == (size_t)length && close_result == 0;
}

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
    if (argc < 2 || argc > 6) {
        fprintf(stderr, "usage: %s <gba-image> [frames [sd-image [capture.ppm [key-events]]]]\n"
                        "  key-events: comma-separated frame:mask taps or first-last:mask holds\n"
                        "    e.g. 1800:0x1 or 1800-1810:0x1\n"
                        "  A26_SUPERCARD_BOARD: lite (default), sd, or chis\n", argv[0]);
        return 2;
    }
    char *end = NULL;
    unsigned long frames = argc >= 3 ? strtoul(argv[2], &end, 10) : 300;
    if ((argc >= 3 && (errno || !end || *end)) || frames == 0 || frames > 1000000) {
        fputs("frame count must be in 1..1000000\n", stderr);
        return 2;
    }

    SuperCardModel model = {0};
    if (!parse_supercard_board(getenv("A26_SUPERCARD_BOARD"), &model.board)) {
        fputs("A26_SUPERCARD_BOARD must be lite, sd, or chis\n", stderr);
        return 2;
    }
    supercard_reset(&model);
    model.sdram_size = 0x02000000u;
    model.sdram = calloc(1, model.sdram_size);
    if (!model.sdram) {
        fputs("could not allocate 32 MiB SuperCard SDRAM\n", stderr);
        return 1;
    }
    if (argc >= 4) {
        model.sd_path = argv[3];
    }

    struct mCore *core = GBACoreCreate();
    SuperCardMGBABus bus = {0};
    int config_initialized = 0;
    if (!core || !core->init(core)) {
        fputs("mGBA GBA core initialization failed\n", stderr);
        if (core) {
            core->deinit(core);
        }
        free(model.sdram);
        return 1;
    }
    mCoreInitConfig(core, NULL);
    config_initialized = 1;
    const char *audio_capture_path = getenv("A26_CAPTURE_AUDIO");

    if (!mCoreLoadFile(core, argv[1])) {
        fprintf(stderr, "mGBA could not load %s\n", argv[1]);
        cleanup(core, &bus, &model, config_initialized);
        return 1;
    }
    if (!stage_kernel_in_sdram(argv[1], &model)) {
        fprintf(stderr, "could not stage kernel image into SuperCard SDRAM: %s\n",
                argv[1]);
        cleanup(core, &bus, &model, config_initialized);
        return 1;
    }
    if (!supercard_mgba_bus_attach(&bus, core, &model)) {
        fputs("could not attach SuperCard memory hooks\n", stderr);
        cleanup(core, &bus, &model, config_initialized);
        return 1;
    }

    unsigned base_width = 0, base_height = 0;
    core->baseVideoSize(core, &base_width, &base_height);
    mColor *video_buffer = calloc((size_t)base_width * base_height,
                                  sizeof(*video_buffer));
    if (!video_buffer || !base_width || !base_height) {
        fputs("could not allocate mGBA video buffer\n", stderr);
        free(video_buffer);
        cleanup(core, &bus, &model, config_initialized);
        return 1;
    }
    core->setVideoBuffer(core, video_buffer, base_width);

    core->reset(core);
    AudioWavCapture audio_capture = {0};
    if (audio_capture_path && *audio_capture_path &&
        !audio_capture_begin(core, audio_capture_path, &audio_capture)) {
        cleanup(core, &bus, &model, config_initialized);
        return 1;
    }
    uint32_t start_frame = core->frameCounter(core);
    struct GBA *gba = core->board;
    SuperCardKeyEvent events[64];
    size_t event_count = 0;
    if (argc == 6) {
        char *event_list = strdup(argv[5]);
        if (!event_list) {
            fputs("could not allocate key-event list\n", stderr);
            free(video_buffer);
            cleanup(core, &bus, &model, config_initialized);
            return 1;
        }
        char *save = NULL;
        for (char *event = strtok_r(event_list, ",", &save); event;
             event = strtok_r(NULL, ",", &save)) {
            char *colon = strchr(event, ':');
            char *range = strchr(event, '-');
            char *first_end = NULL, *last_end = NULL, *mask_end = NULL;
            if (!colon || event_count == 64 || (range && range > colon)) {
                free(event_list);
                fputs("invalid key-event list (maximum 64 frame or range entries)\n", stderr);
                free(video_buffer);
                cleanup(core, &bus, &model, config_initialized);
                return 2;
            }
            *colon++ = '\0';
            unsigned long first_frame, last_frame;
            errno = 0;
            if (range) {
                *range++ = '\0';
                first_frame = strtoul(event, &first_end, 10);
                last_frame = strtoul(range, &last_end, 10);
            } else {
                first_frame = strtoul(event, &first_end, 10);
                last_frame = first_frame;
                last_end = first_end;
            }
            unsigned long mask = strtoul(colon, &mask_end, 0);
            if (errno || !first_end || *first_end || !last_end || *last_end ||
                !mask_end || *mask_end || first_frame > last_frame ||
                last_frame >= frames || mask > 0x3FFu) {
                free(event_list);
                fputs("key event range/mask is out of range or malformed\n", stderr);
                free(video_buffer);
                cleanup(core, &bus, &model, config_initialized);
                return 2;
            }
            events[event_count++] = (SuperCardKeyEvent) {
                .first_frame = first_frame,
                .last_frame = last_frame,
                .mask = (uint32_t)mask,
            };
        }
        free(event_list);
    }
    const char *previous_region = NULL;
    int trace_first_bios = getenv("A26_TRACE_FIRST_BIOS") != NULL;
    int trace_pc_frames = getenv("A26_TRACE_PC_FRAMES") != NULL;
    int trace_reset_tail = getenv("A26_TRACE_RESET_TAIL") != NULL;
    int trace_atari_globals = getenv("A26_TRACE_ATARI_GLOBALS") != NULL;
    unsigned long atari_trace_interval = 0;
    const char *atari_interval_text = getenv("A26_TRACE_ATARI_INTERVAL");
    if (trace_atari_globals) {
        atari_trace_interval = atari_interval_text ?
            strtoul(atari_interval_text, NULL, 10) : 1ul;
        if (!atari_trace_interval) {
            atari_trace_interval = 1ul;
        }
    }
    unsigned atari_pc_offset = env_offset("A26_ATARI_PC_OFFSET", 0x5D90u);
    unsigned atari_clk_offset = env_offset("A26_ATARI_CLK_OFFSET", 0x5D84u);
    unsigned atari_clkcount_offset = env_offset("A26_ATARI_CLKCOUNT_OFFSET", 0x5BFCu);
    unsigned atari_tv_counter_offset = env_offset("A26_ATARI_TV_COUNTER_OFFSET", 0x5C0Cu);
    unsigned atari_ebeamy_offset = env_offset("A26_ATARI_EBEAMY_OFFSET", 0x679Cu);
    unsigned atari_hbeam_offset = env_offset("A26_ATARI_HBEAM_OFFSET", 0x6468u);
    unsigned atari_vbeam_offset = env_offset("A26_ATARI_VBEAM_OFFSET", 0x6474u);
    unsigned atari_pace_offset = env_offset("A26_ATARI_PACE_OFFSET", 0x7A50u);
    unsigned long stop_atari_tv_counter = 0;
    const char *stop_tv_text = getenv("A26_STOP_ATARI_TV_COUNTER");
    if (stop_tv_text && *stop_tv_text) {
        char *end = NULL;
        unsigned long value = strtoul(stop_tv_text, &end, 0);
        if (end && !*end && value <= UINT32_MAX) {
            stop_atari_tv_counter = value;
        }
    }
    int reset_tail_traced = 0;
    int pc_frame_window = 0;
    int first_bios_traced = 0;
    int atari_counter_started = 0;
    int precise_atari_stop = 0;
    uint32_t atari_stop_cycles = 0;
    uint32_t gba_cycles_start = (uint32_t)mTimingCurrentTime(&gba->timing);
    uint32_t atari_last_tv_counter = 0;
    uint64_t atari_display_calls = 0;
    uint64_t atari_rendered_calls = 0;
    uint32_t min_atari_iwram_sp = UINT32_MAX;
    for (unsigned long i = 0; i < frames; ++i) {
        uint32_t keys = 0;
        for (size_t event = 0; event < event_count; ++event) {
            if (events[event].first_frame <= i && i <= events[event].last_frame) {
                keys = events[event].mask;
                break;
            }
        }
        core->setKeys(core, keys);
        if (stop_atari_tv_counter && trace_atari_globals) {
            uint32_t frame_before = core->frameCounter(core);
            const uint8_t *iwram = (const uint8_t *)gba->memory.iwram;
            while (core->frameCounter(core) == frame_before) {
                core->runLoop(core);
                int32_t stop_pc = -1;
                if (!precise_atari_stop &&
                    core->readRegister(core, "pc", &stop_pc) &&
                    strcmp(pc_region((uint32_t)stop_pc), "cartridge") == 0 &&
                    read_le32(iwram, atari_tv_counter_offset) >=
                        stop_atari_tv_counter) {
                    precise_atari_stop = 1;
                    atari_stop_cycles =
                        (uint32_t)mTimingCurrentTime(&gba->timing) -
                        gba_cycles_start;
                    printf("atari_stop precise=1 frame=%lu gba_cycles=%" PRIu32
                           " cpu_pc=0x%04" PRIX32 " clk=%" PRIu32
                           " clkcount=%" PRIu32 " tv_counter=%" PRIu32
                           " ebeamy=%" PRIu32 " pace_frame=%" PRIu32 "\n",
                           i + 1u, atari_stop_cycles,
                           read_le32(iwram, atari_pc_offset) & 0xFFFFu,
                           read_le32(iwram, atari_clk_offset),
                           read_le32(iwram, atari_clkcount_offset),
                           read_le32(iwram, atari_tv_counter_offset),
                           read_le32(iwram, atari_ebeamy_offset),
                           read_le32(iwram, atari_pace_offset));
                }
            }
        } else {
            core->runFrame(core);
        }
        if (audio_capture.file && !audio_capture_drain(core, &audio_capture)) {
            fclose(audio_capture.file);
            audio_capture.file = NULL;
            free(video_buffer);
            cleanup(core, &bus, &model, config_initialized);
            return 1;
        }
        int32_t trace_pc = -1;
        if (core->readRegister(core, "pc", &trace_pc)) {
            const char *region = pc_region((uint32_t)trace_pc);
            if (trace_atari_globals && strcmp(region, "cartridge") == 0) {
                const uint8_t *iwram = (const uint8_t *)gba->memory.iwram;
                int32_t sp = -1;
                if (atari_counter_started && core->readRegister(core, "sp", &sp) &&
                    (uint32_t)sp >= 0x03000000u &&
                    (uint32_t)sp < min_atari_iwram_sp) {
                    min_atari_iwram_sp = (uint32_t)sp;
                }
                uint32_t tv_counter = read_le32(iwram, atari_tv_counter_offset);
                uint32_t pace_frame = read_le32(iwram, atari_pace_offset);
                if (!atari_counter_started) {
                    atari_last_tv_counter = tv_counter;
                    atari_counter_started = 1;
                } else if (tv_counter != atari_last_tv_counter) {
                    uint32_t delta = tv_counter - atari_last_tv_counter;
                    atari_display_calls += delta;
                    if (pace_frame) {
                        atari_rendered_calls += delta;
                    }
                    atari_last_tv_counter = tv_counter;
                }
                if ((i + 1u) % atari_trace_interval == 0 || keys != 0) {
                    printf("atari_probe frame=%lu keys=0x%03" PRIX32
                           " cpu_pc=0x%04" PRIX32 " clk=%" PRIu32
                           " clkcount=%" PRIu32 " tv_counter=%" PRIu32
                           " ebeamy=%" PRIu32 " hbeam=%" PRIu32
                           " vbeam=%" PRIu32 " pace_frame=%" PRIu32
                           " display_calls=%" PRIu64
                           " rendered_calls=%" PRIu64 "\n",
                           i + 1u, keys,
                           read_le32(iwram, atari_pc_offset) & 0xFFFFu,
                           read_le32(iwram, atari_clk_offset),
                           read_le32(iwram, atari_clkcount_offset), tv_counter,
                           read_le32(iwram, atari_ebeamy_offset),
                           read_le32(iwram, atari_hbeam_offset),
                           read_le32(iwram, atari_vbeam_offset), pace_frame,
                           atari_display_calls, atari_rendered_calls);
                }
            }
            if (trace_reset_tail && !reset_tail_traced &&
                (uint32_t)trace_pc == 0x03007E2Cu) {
                fputs("reset_tail_trace_start\n", stdout);
                const char *step_region = "";
                int cartridge_trace = 0;
                int copied_header_trace = 0;
                for (unsigned step = 0; step < 250000; ++step) {
                    core->readRegister(core, "pc", &trace_pc);
                    uint32_t pc_value = (uint32_t)trace_pc;
                    const char *current_region = pc_region(pc_value);
                    if (strcmp(current_region, "cartridge") == 0) {
                        cartridge_trace = 1;
                    }
                    if (cartridge_trace && pc_value >= 0x03800000u &&
                        pc_value < 0x03800400u) {
                        if (!copied_header_trace) {
                            const uint8_t *iwram =
                                (const uint8_t *)gba->memory.iwram;
                            fputs("reset_tail_copied_header_words=", stdout);
                            for (unsigned offset = 0x130u; offset <= 0x1A4u;
                                 offset += 4u) {
                                uint32_t value = (uint32_t)iwram[offset] |
                                    ((uint32_t)iwram[offset + 1u] << 8) |
                                    ((uint32_t)iwram[offset + 2u] << 16) |
                                    ((uint32_t)iwram[offset + 3u] << 24);
                                printf("%s%03X:%08" PRIX32,
                                       offset == 0x130u ? "" : ",", offset,
                                       value);
                            }
                            putchar('\n');
                        }
                        copied_header_trace = 1;
                    }
                    int32_t trace_lr = 0, trace_sp = 0, trace_r0 = 0;
                    int32_t trace_r1 = 0, trace_r9 = 0, trace_r10 = 0;
                    int32_t trace_r11 = 0;
                    if (cartridge_trace || step % 10000u == 0 ||
                        strcmp(current_region, step_region) != 0 ||
                        pc_value == 0x03007E68u || pc_value == 0x03007EB8u ||
                        pc_value == 0x0380018Cu || pc_value == 0xB0DC30B6u) {
                        core->readRegister(core, "lr", &trace_lr);
                        core->readRegister(core, "sp", &trace_sp);
                        core->readRegister(core, "r0", &trace_r0);
                        core->readRegister(core, "r1", &trace_r1);
                    }
                    if (step % 10000u == 0 ||
                        strcmp(current_region, step_region) != 0 ||
                        pc_value == 0x03007E68u || pc_value == 0x03007EB8u ||
                        pc_value == 0x0380018Cu || pc_value == 0xB0DC30B6u) {
                        int32_t trace_r9 = 0, trace_r10 = 0, trace_r11 = 0;
                        core->readRegister(core, "r9", &trace_r9);
                        core->readRegister(core, "r10", &trace_r10);
                        core->readRegister(core, "r11", &trace_r11);
                        printf("reset_tail_step=%u pc=%08" PRIX32
                               " lr=%08" PRIX32 " sp=%08" PRIX32
                               " r0=%08" PRIX32 " r1=%08" PRIX32
                               " r9=%08" PRIX32 " r10=%08" PRIX32
                               " r11=%08" PRIX32 "\n",
                               step, pc_value, (uint32_t)trace_lr,
                               (uint32_t)trace_sp, (uint32_t)trace_r0,
                               (uint32_t)trace_r1, (uint32_t)trace_r9,
                               (uint32_t)trace_r10, (uint32_t)trace_r11);
                    }
                    if (cartridge_trace &&
                        (strcmp(current_region, "cartridge") == 0 ||
                         copied_header_trace)) {
                        int32_t trace_r2 = 0, trace_r3 = 0;
                        core->readRegister(core, "r2", &trace_r2);
                        core->readRegister(core, "r3", &trace_r3);
                        printf("reset_tail_flow_step=%u pc=%08" PRIX32
                               " lr=%08" PRIX32 " sp=%08" PRIX32
                               " r0=%08" PRIX32 " r1=%08" PRIX32
                               " r2=%08" PRIX32 " r3=%08" PRIX32 "\n",
                               step, pc_value, (uint32_t)trace_lr,
                               (uint32_t)trace_sp, (uint32_t)trace_r0,
                               (uint32_t)trace_r1, (uint32_t)trace_r2,
                               (uint32_t)trace_r3);
                        fflush(stdout);
                    }
                    step_region = current_region;
                    if (copied_header_trace &&
                        !(pc_value >= 0x03800000u && pc_value < 0x03800400u)) {
                        printf("reset_tail_copied_header_exit_step=%u pc=%08"
                               PRIX32 " region=%s\n", step, pc_value,
                               current_region);
                        break;
                    }
                    core->step(core);
                }
                reset_tail_traced = 1;
                break;
            }
            if (trace_pc_frames && !pc_frame_window && i > frames / 2u &&
                (uint32_t)trace_pc >= 0x03007E00u &&
                (uint32_t)trace_pc < 0x03008000u) {
                pc_frame_window = 1;
            }
            if (trace_pc_frames && pc_frame_window) {
                int32_t trace_lr = 0, trace_sp = 0;
                core->readRegister(core, "lr", &trace_lr);
                core->readRegister(core, "sp", &trace_sp);
                printf("pc_frame frame=%lu pc=0x%08" PRIX32
                       " lr=0x%08" PRIX32 " sp=0x%08" PRIX32 "\n",
                       i + 1u, (uint32_t)trace_pc, (uint32_t)trace_lr,
                       (uint32_t)trace_sp);
                if (region == pc_region(0x03000000u +
                                         ((uint32_t)trace_pc & 0x7FFFu))) {
                    uint32_t offset = (uint32_t)trace_pc & 0x7FFFu;
                    if (offset <= 0x7FF0u) {
                        fputs("pc_frame_iwram_words=", stdout);
                        for (unsigned word = 0; word < 4; ++word) {
                            uint32_t value = 0;
                            for (unsigned byte = 0; byte < 4; ++byte) {
                                value |= (uint32_t)((const uint8_t *)gba->memory.iwram)
                                         [offset + word * 4u + byte]
                                         << (byte * 8u);
                            }
                            printf("%s%08" PRIX32, word ? "," : "", value);
                        }
                        putchar('\n');
                    }
                }
            }
            if (!previous_region || strcmp(region, previous_region) != 0) {
                printf("pc_region frame=%lu region=%s pc=0x%08" PRIX32 "\n",
                       i + 1, region, (uint32_t)trace_pc);
                previous_region = region;
            }
            if (trace_first_bios && !first_bios_traced && i > frames / 2u &&
                strcmp(region, "bios") == 0) {
                int32_t trace_lr = 0, trace_sp = 0, trace_r0 = 0, trace_r1 = 0;
                core->readRegister(core, "lr", &trace_lr);
                core->readRegister(core, "sp", &trace_sp);
                core->readRegister(core, "r0", &trace_r0);
                core->readRegister(core, "r1", &trace_r1);
                printf("first_bios frame=%lu pc=0x%08" PRIX32
                       " lr=0x%08" PRIX32 " sp=0x%08" PRIX32
                       " r0=0x%08" PRIX32 " r1=0x%08" PRIX32 "\n",
                       i + 1, (uint32_t)trace_pc, (uint32_t)trace_lr,
                       (uint32_t)trace_sp, (uint32_t)trace_r0,
                       (uint32_t)trace_r1);
                uint32_t stack_offset = (uint32_t)trace_sp & 0x7FFFu;
                if ((uint32_t)trace_sp >= 0x03000000u &&
                    (uint32_t)trace_sp < 0x04000000u && stack_offset >= 16u &&
                    stack_offset <= 0x7FF0u) {
                    fputs("first_bios_stack=", stdout);
                    for (unsigned word = 0; word < 8; ++word) {
                        uint32_t value = 0;
                        for (unsigned byte = 0; byte < 4; ++byte) {
                            value |= (uint32_t)((const uint8_t *)gba->memory.iwram)
                                     [stack_offset - 16u + word * 4u + byte]
                                     << (byte * 8u);
                        }
                        printf("%s%08" PRIX32, word ? "," : "", value);
                    }
                    putchar('\n');
                }
                for (unsigned step = 0; step < 24; ++step) {
                    core->readRegister(core, "pc", &trace_pc);
                    core->readRegister(core, "lr", &trace_lr);
                    core->readRegister(core, "sp", &trace_sp);
                    printf("first_bios_step=%u pc=0x%08" PRIX32
                           " lr=0x%08" PRIX32 " sp=0x%08" PRIX32 "\n",
                           step, (uint32_t)trace_pc, (uint32_t)trace_lr,
                           (uint32_t)trace_sp);
                    core->step(core);
                }
                first_bios_traced = 1;
                break;
            }
            if (trace_pc_frames && pc_frame_window &&
                strcmp(region, "bios") == 0) {
                fputs("pc_frame_trace_stop=first_bios\n", stdout);
                break;
            }
        }
        if (precise_atari_stop) break;
    }
    const uint8_t *ewram = (const uint8_t *)gba->memory.wram;
    const uint8_t *iwram = (const uint8_t *)gba->memory.iwram;
    if (getenv("A26_TRACE_SOFTRESET")) {
        for (unsigned step = 0; step < 24; ++step) {
            int32_t trace_pc = -1, trace_lr = 0, trace_r0 = 0, trace_r1 = 0;
            int32_t trace_sp = 0;
            core->readRegister(core, "pc", &trace_pc);
            core->readRegister(core, "lr", &trace_lr);
            core->readRegister(core, "r0", &trace_r0);
            core->readRegister(core, "r1", &trace_r1);
            core->readRegister(core, "sp", &trace_sp);
            printf("softreset_step=%u pc=0x%08" PRIX32 " lr=0x%08" PRIX32
                   " sp=0x%08" PRIX32 " r0=0x%08" PRIX32
                   " r1=0x%08" PRIX32,
                   step, (uint32_t)trace_pc, (uint32_t)trace_lr,
                   (uint32_t)trace_sp, (uint32_t)trace_r0, (uint32_t)trace_r1);
            uint32_t stack_offset = (uint32_t)trace_sp & 0x7FFFu;
            if ((uint32_t)trace_sp >= 0x03000000u &&
                (uint32_t)trace_sp < 0x04000000u && stack_offset >= 16u &&
                stack_offset <= 0x7FF0u) {
                fputs(" stack=", stdout);
                for (unsigned word = 0; word < 8; ++word) {
                    uint32_t value = 0;
                    for (unsigned byte = 0; byte < 4; ++byte) {
                        value |= (uint32_t)iwram[stack_offset - 16u +
                                                word * 4u + byte]
                                 << (byte * 8u);
                    }
                    printf("%s%08" PRIX32, word ? "," : "", value);
                }
            }
            putchar('\n');
            core->step(core);
        }
    }
    uint64_t frame_hash = 0;
    unsigned video_width = 0, video_height = 0;
    const char *capture_path = argc >= 5 ? argv[4] : NULL;
    if (!capture_frame(core, capture_path, &frame_hash,
                       &video_width, &video_height)) {
        fputs("could not capture mGBA GBA framebuffer\n", stderr);
        free(video_buffer);
        cleanup(core, &bus, &model, config_initialized);
        return 1;
    }
    uint32_t end_frame = core->frameCounter(core);
    if (trace_atari_globals) {
        printf("atari_probe_total display_calls=%" PRIu64
               " rendered_calls=%" PRIu64 " min_iwram_sp=0x%08" PRIX32 "\n",
               atari_display_calls, atari_rendered_calls,
               min_atari_iwram_sp);
    }
    int32_t pc = -1, r0 = 0, r1 = 0, r2 = 0, r3 = 0, lr = 0;
    int has_pc = core->readRegister(core, "pc", &pc);
    core->readRegister(core, "r0", &r0);
    core->readRegister(core, "r1", &r1);
    core->readRegister(core, "r2", &r2);
    core->readRegister(core, "r3", &r3);
    core->readRegister(core, "lr", &lr);
    printf("mGBA platform=GBA requested_frames=%lu frame_counter_delta=%" PRIu32
           " pc=%s0x%08" PRIX32 " r0=%08" PRIX32 " r1=%08" PRIX32
           " r2=%08" PRIX32 " r3=%08" PRIX32 " lr=%08" PRIX32
           " mode_register_writes=%" PRIu64 " magic_writes=%" PRIu64
           " mode_words=%" PRIu64 " last_mode=0x%04X sd_command_writes=%" PRIu64
           " sd_data_writes=%" PRIu64 " cartridge_writes=%" PRIu64
           " last_cartridge_write=0x%08" PRIX32 " board=%s map=%s sd=%d write=%d"
           " framebuffer=%ux%u fnv64=%016" PRIX64 " capture=%s\n",
           frames, end_frame - start_frame, has_pc ? "" : "unavailable:",
           has_pc ? (uint32_t) pc : 0, (uint32_t) r0, (uint32_t) r1,
           (uint32_t) r2, (uint32_t) r3, (uint32_t) lr,
           bus.mode_register_writes,
           bus.mode_magic_writes, bus.mode_value_writes, bus.last_mode_word,
           bus.sd_command_writes, bus.sd_data_writes,
           bus.cartridge_writes, bus.last_cartridge_write,
           supercard_board_name(model.board),
           model.mode == SUPERCARD_MODE_SDRAM ? "sdram" : "firmware",
           model.sd_interface, model.write_access, video_width, video_height,
           frame_hash, capture_path ? capture_path : "(not-written)");
    printf("gba_timing waitcnt=0x%04X rom0_nonseq16=%d rom0_seq16=%d"
           " active_nonseq16=%d active_seq16=%d\n",
           gba->memory.io[GBA_REG_WAITCNT >> 1],
           gba->memory.waitstatesNonseq16[0x08],
           gba->memory.waitstatesSeq16[0x08],
           gba->cpu->memory.activeNonseqCycles16,
           gba->cpu->memory.activeSeqCycles16);
    printf("sd_protocol decoded_commands=%" PRIu64 " last_command=%u"
           " responses=%" PRIu64 " response_line_reads=%" PRIu64
           " data_port_reads=%" PRIu64 " argument=0x%08" PRIX32
           " data_active=%d data_lba=%" PRIu32 " block_loads=%" PRIu64
           " load_failures=%" PRIu64 " token_polls=%" PRIu64 " image=%s\n",
           model.sd_commands_decoded, model.sd_last_command,
           model.sd_responses_issued, model.sd_response_bits_read,
           bus.sd_data_reads, model.sd_last_argument, model.sd_data_active,
           model.sd_data_lba, model.sd_data_blocks_loaded,
           model.sd_data_load_failures, model.sd_data_token_polls,
           model.sd_path ? model.sd_path : "(none)");
    printf("sd_block_commands read=%" PRIu64 " write=%" PRIu64
           " written=%" PRIu64 " write_failures=%" PRIu64
           " stop=%" PRIu64 "\n",
           model.sd_read_commands, model.sd_write_commands,
           model.sd_write_blocks, model.sd_write_failures,
           model.sd_stop_commands);
    fputs("sdram_window=", stdout);
    for (unsigned i = 0; i < 16; ++i) {
        printf("%02X", model.sdram[i]);
    }
    putchar('\n');
    printf("reset_state iwram_softreset_flag=%u ewram_entry=",
           iwram[0x7FFAu]);
    for (unsigned i = 0; i < 16; ++i) {
        printf("%02X", ewram[i]);
    }
    printf(" sdram_entry=");
    for (unsigned i = 0; i < 16; ++i) {
        printf("%02X", model.sdram[i]);
    }
    putchar('\n');
    printf("reset_stack_write_trace total=%" PRIu32 " entries=%" PRIu32,
           bus.reset_stack_trace_total, bus.reset_stack_trace_count);
    uint32_t first_stack_write = bus.reset_stack_trace_total >=
                                 SUPERCARD_RESET_TRACE_CAPACITY ?
                                 bus.reset_stack_trace_total %
                                     SUPERCARD_RESET_TRACE_CAPACITY : 0;
    for (uint32_t i = 0; i < bus.reset_stack_trace_count; ++i) {
        const SuperCardResetStackWrite *write =
            &bus.reset_stack_trace[(first_stack_write + i) %
                                   SUPERCARD_RESET_TRACE_CAPACITY];
        printf(" [pc=%08" PRIX32 " addr=%08" PRIX32 " width=%u value=%08"
               PRIX32 "]", write->pc, write->address, write->width,
               write->value);
    }
    putchar('\n');
    printf("cmd_write_trace count=%" PRIu32 ":", bus.cmd_write_trace_count);
    for (uint32_t i = 0; i < bus.cmd_write_trace_count; ++i) {
        printf(" %02X", bus.cmd_write_trace[i] & 0xFFu);
    }
    putchar('\n');

    if (audio_capture.file && !audio_capture_finish(core, &audio_capture)) {
        free(video_buffer);
        cleanup(core, &bus, &model, config_initialized);
        return 1;
    }

    free(video_buffer);
    cleanup(core, &bus, &model, config_initialized);
    return 0;
}
