#include "supercard_mgba_bus.h"

#include <mgba/core/config.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/gba/core.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/input.h>
#include <mgba-util/image.h>

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

static const char *pc_region(uint32_t pc) {
    if (pc < 0x00004000u) return "bios";
    if (pc >= 0x02000000u && pc < 0x03000000u) return "ewram";
    if (pc >= 0x03000000u && pc < 0x04000000u) return "iwram";
    if (pc >= 0x08000000u && pc < 0x0E000000u) return "cartridge";
    return "other";
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
                        "  key-events: comma-separated frame:mask taps (e.g. 1800:0x1)\n", argv[0]);
        return 2;
    }
    char *end = NULL;
    unsigned long frames = argc >= 3 ? strtoul(argv[2], &end, 10) : 300;
    if ((argc >= 3 && (errno || !end || *end)) || frames == 0 || frames > 1000000) {
        fputs("frame count must be in 1..1000000\n", stderr);
        return 2;
    }

    SuperCardModel model = {0};
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
    uint32_t start_frame = core->frameCounter(core);
    struct GBA *gba = core->board;
    unsigned long event_frame[64];
    uint32_t event_mask[64];
    size_t event_count = 0;
    if (argc == 6) {
        char *events = strdup(argv[5]);
        if (!events) {
            fputs("could not allocate key-event list\n", stderr);
            free(video_buffer);
            cleanup(core, &bus, &model, config_initialized);
            return 1;
        }
        char *save = NULL;
        for (char *event = strtok_r(events, ",", &save); event;
             event = strtok_r(NULL, ",", &save)) {
            char *colon = strchr(event, ':');
            char *frame_end = NULL, *mask_end = NULL;
            if (!colon || event_count == 64) {
                free(events);
                fputs("invalid key-event list (maximum 64 frame:mask entries)\n", stderr);
                free(video_buffer);
                cleanup(core, &bus, &model, config_initialized);
                return 2;
            }
            *colon++ = '\0';
            errno = 0;
            unsigned long event_at = strtoul(event, &frame_end, 10);
            unsigned long mask = strtoul(colon, &mask_end, 0);
            if (errno || !frame_end || *frame_end || !mask_end || *mask_end ||
                event_at >= frames || mask > 0x3FFu) {
                free(events);
                fputs("key event frame/mask is out of range or malformed\n", stderr);
                free(video_buffer);
                cleanup(core, &bus, &model, config_initialized);
                return 2;
            }
            event_frame[event_count] = event_at;
            event_mask[event_count++] = (uint32_t)mask;
        }
        free(events);
    }
    const char *previous_region = NULL;
    int trace_first_bios = getenv("A26_TRACE_FIRST_BIOS") != NULL;
    int trace_pc_frames = getenv("A26_TRACE_PC_FRAMES") != NULL;
    int trace_reset_tail = getenv("A26_TRACE_RESET_TAIL") != NULL;
    int reset_tail_traced = 0;
    int pc_frame_window = 0;
    int first_bios_traced = 0;
    for (unsigned long i = 0; i < frames; ++i) {
        uint32_t keys = 0;
        for (size_t event = 0; event < event_count; ++event) {
            if (event_frame[event] == i) {
                keys = event_mask[event];
                break;
            }
        }
        core->setKeys(core, keys);
        core->runFrame(core);
        int32_t trace_pc = -1;
        if (core->readRegister(core, "pc", &trace_pc)) {
            const char *region = pc_region((uint32_t)trace_pc);
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
           " last_cartridge_write=0x%08" PRIX32 " map=%s sd=%d write=%d"
           " framebuffer=%ux%u fnv64=%016" PRIX64 " capture=%s\n",
           frames, end_frame - start_frame, has_pc ? "" : "unavailable:",
           has_pc ? (uint32_t) pc : 0, (uint32_t) r0, (uint32_t) r1,
           (uint32_t) r2, (uint32_t) r3, (uint32_t) lr,
           bus.mode_register_writes,
           bus.mode_magic_writes, bus.mode_value_writes, bus.last_mode_word,
           bus.sd_command_writes, bus.sd_data_writes,
           bus.cartridge_writes, bus.last_cartridge_write,
           model.mode == SUPERCARD_MODE_SDRAM ? "sdram" : "firmware",
           model.sd_interface, model.write_access, video_width, video_height,
           frame_hash, capture_path ? capture_path : "(not-written)");
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

    free(video_buffer);
    cleanup(core, &bus, &model, config_initialized);
    return 0;
}
