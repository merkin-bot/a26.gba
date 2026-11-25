#include "ezflash_mgba_sd.h"

#include <mgba/core/config.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/gba/core.h>
#include <mgba/internal/arm/arm.h>
#include <mgba/internal/gba/gba.h>
#include <mgba-util/vfs.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

#define DATA 0x09E00000u

static void quiet_log(struct mLogger *logger, int category,
                      enum mLogLevel level, const char *format, va_list args) {
    (void)logger; (void)category; (void)level; (void)format; (void)args;
}

static void issue_command(struct mCore *core, uint32_t sector,
                          uint16_t count_word) {
    core->busWrite16(core, 0x09FE0000u, 0xD200u);
    core->busWrite16(core, 0x08000000u, 0x1500u);
    core->busWrite16(core, 0x08020000u, 0xD200u);
    core->busWrite16(core, 0x08040000u, 0x1500u);
    core->busWrite16(core, 0x09600000u, (uint16_t)sector);
    core->busWrite16(core, 0x09620000u, (uint16_t)(sector >> 16));
    core->busWrite16(core, 0x09640000u, count_word);
    core->busWrite16(core, 0x09FC0000u, 0x1500u);
}

static void set_control(struct mCore *core, uint16_t control) {
    core->busWrite16(core, 0x09FE0000u, 0xD200u);
    core->busWrite16(core, 0x08000000u, 0x1500u);
    core->busWrite16(core, 0x08020000u, 0xD200u);
    core->busWrite16(core, 0x08040000u, 0x1500u);
    core->busWrite16(core, 0x09400000u, control);
    core->busWrite16(core, 0x09FC0000u, 0x1500u);
}

static void set_psram_page(struct mCore *core, uint16_t page) {
    core->busWrite16(core, 0x09FE0000u, 0xD200u);
    core->busWrite16(core, 0x08000000u, 0x1500u);
    core->busWrite16(core, 0x08020000u, 0xD200u);
    core->busWrite16(core, 0x08040000u, 0x1500u);
    core->busWrite16(core, 0x09860000u, page);
    core->busWrite16(core, 0x09FC0000u, 0x1500u);
}

static void set_rom_page(struct mCore *core, uint16_t page) {
    core->busWrite16(core, 0x09FE0000u, 0xD200u);
    core->busWrite16(core, 0x08000000u, 0x1500u);
    core->busWrite16(core, 0x08020000u, 0xD200u);
    core->busWrite16(core, 0x08040000u, 0x1500u);
    core->busWrite16(core, 0x09880000u, page);
    core->busWrite16(core, 0x09FC0000u, 0x1500u);
}

int main(void) {
    mLogSetDefaultLogger(&(struct mLogger){.log = quiet_log});
    char path[] = "/tmp/ezflash-mgba-XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) return 1;
    FILE *file = fdopen(fd, "w+b");
    if (!file) return 1;
    uint8_t input[2048] = {0};
    for (unsigned i = 0; i < sizeof(input); ++i) input[i] = (uint8_t)(i ^ 0x6D);
    if (fwrite(input, 1, sizeof(input), file) != sizeof(input)) return 1;
    fclose(file);

    EZFlashSDModel model = {0};
    EZFlashPSRAMModel psram = {0};
    EZFlashNORModel nor = {0};
    EZFlashNORModel nor_s98 = {0};
    EZFlashMGBASD adapter = {0};
    struct mCore *core = GBACoreCreate();
    int config_initialized = 0;
    int result = 1;
    if (!core || !core->init(core)) goto done;
    mCoreInitConfig(core, NULL);
    config_initialized = 1;
    core->reset(core);
    struct VFile *rom_file = VFileOpen(path, O_RDONLY);
    if (!rom_file || !core->loadROM(core, rom_file)) {
        if (rom_file) rom_file->close(rom_file);
        fputs("smoke ROM load failed\n", stderr); goto done;
    }
    if (!ezflash_sd_model_open(&model, path, 0) ||
        !ezflash_psram_model_init(&psram) ||
        !ezflash_nor_model_init(&nor, path) ||
        !ezflash_nor_model_init_window(&nor_s98, EZFLASH_S98_NOR_BASE,
                                       0x20000u, NULL, 0xFFFFu) ||
        !ezflash_mgba_sd_attach(&adapter, core, &model, &psram, &nor,
                                &nor_s98)) {
        fputs("smoke model/adapter setup failed\n", stderr); goto done;
    }

    set_control(core, 1);
    issue_command(core, 1, 1);
    set_control(core, 3);
    if (core->busRead16(core, DATA) != 0) goto done;
    int original_cycles = 0, synthetic_cycles = 0;
    uint32_t (*original_load32)(struct ARMCore *, uint32_t, int *) = adapter.load32;
    struct ARMCore *cpu = core->cpu;
    uint32_t original_value = original_load32(cpu, DATA, &original_cycles);
    uint32_t synthetic_value = cpu->memory.load32(cpu, DATA, &synthetic_cycles);
    uint32_t expected_first = (uint32_t)input[512] |
        (uint32_t)input[513] << 8 | (uint32_t)input[514] << 16 |
        (uint32_t)input[515] << 24;
    (void)original_value;
    if (synthetic_value != expected_first || original_cycles != synthetic_cycles)
        goto done;
    int transfer_cycles = 0;
    (void)cpu->memory.loadMultiple(cpu, DATA, 0x3, LSM_IA, &transfer_cycles);
    uint32_t expected_second = (uint32_t)input[516] |
        (uint32_t)input[517] << 8 | (uint32_t)input[518] << 16 |
        (uint32_t)input[519] << 24;
    if (cpu->gprs[0] != expected_first || cpu->gprs[1] != expected_second)
        goto done;
    for (unsigned i = 0; i < 512; i += 4) {
        uint32_t expected = (uint32_t)input[512 + i] |
            (uint32_t)input[513 + i] << 8 |
            (uint32_t)input[514 + i] << 16 |
            (uint32_t)input[515 + i] << 24;
        if (core->busRead32(core, DATA + i) != expected) goto done;
    }

    /* The kernel uses GBA DMA to transfer this aperture into EWRAM. */
    set_control(core, 1);
    issue_command(core, 1, 1);
    set_control(core, 3);
    if (core->busRead16(core, DATA) != 0) goto done;
    core->busWrite32(core, 0x040000D4u, DATA);          /* DMA3 source */
    core->busWrite32(core, 0x040000D8u, 0x02000000u);  /* DMA3 destination */
    core->busWrite16(core, 0x040000DCu, 256);          /* 512 bytes */
    core->busWrite16(core, 0x040000DEu, 0x8000u);      /* immediate, 16-bit */
    core->runFrame(core);
    if (memcmp(((struct GBA *)core->board)->memory.wram, input + 512, 512) != 0) {
        struct GBA *gba = core->board;
        fprintf(stderr, "DMA SD mismatch blocks=%llu reg=%04X count=%u next=%u first=%08X expected=%02X%02X%02X%02X\n",
                (unsigned long long)model.blocks_read, gba->memory.dma[3].reg,
                gba->memory.dma[3].count, gba->memory.dma[3].nextCount,
                *(uint32_t *)gba->memory.wram, input[515], input[514], input[513], input[512]);
        goto done;
    }

    core->busWrite32(core, EZFLASH_PSRAM_BASE, (int32_t)0x11223344u);
    set_psram_page(core, 0x0800u); /* Source page step for a 4 MiB window. */
    core->busWrite32(core, EZFLASH_PSRAM_BASE, (int32_t)0xA1B2C3D4u);
    set_psram_page(core, 0);
    if (core->busRead32(core, EZFLASH_PSRAM_BASE) != 0x11223344u) goto done;
    set_psram_page(core, 0x0800u);
    if (core->busRead32(core, EZFLASH_PSRAM_BASE) != 0xA1B2C3D4u ||
        psram.page_writes < 3 || !adapter.psram_reads || !adapter.psram_writes)
        goto done;
    set_rom_page(core, 0x0201u); /* BLUE's source adds SYSTEM_MODE_OS bit 0. */
    if (core->busRead32(core, EZFLASH_S71_NOR_BASE) != 0x11223344u ||
        ((struct GBA *)core->board)->memory.rom[0] != 0x11223344u) {
        fprintf(stderr, "smoke ROM map mismatch bus=%08X rom=%08X\n",
                core->busRead32(core, EZFLASH_S71_NOR_BASE),
                ((struct GBA *)core->board)->memory.rom[0]); goto done;
    }
    set_rom_page(core, 0x0400u); /* BLUE's plugin handoff selects this PSRAM page. */
    if (core->busRead32(core, EZFLASH_S71_NOR_BASE) != 0x11223344u ||
        ((struct GBA *)core->board)->memory.rom[0] != 0x11223344u) goto done;
    set_rom_page(core, 0);
    if (core->busRead32(core, EZFLASH_S71_NOR_BASE) != 0x6E6F6C6Du ||
        ((struct GBA *)core->board)->memory.rom[0] != 0x6E6F6C6Du) goto done;

    /* BLUE's source keeps its save-info NOR window distinct at 0x09000000
     * and documents 128 KiB erase sectors there. */
    uint32_t s98_target = EZFLASH_S98_NOR_BASE + 0x790000u;
    core->busWrite16(core, EZFLASH_S98_NOR_BASE + 0xAAAu, 0x00AAu);
    core->busWrite16(core, EZFLASH_S98_NOR_BASE + 0x554u, 0x0055u);
    core->busWrite16(core, EZFLASH_S98_NOR_BASE + 0xAAAu, 0x0080u);
    core->busWrite16(core, EZFLASH_S98_NOR_BASE + 0xAAAu, 0x00AAu);
    core->busWrite16(core, EZFLASH_S98_NOR_BASE + 0x554u, 0x0055u);
    core->busWrite16(core, s98_target, 0x0030u);
    core->busWrite16(core, EZFLASH_S98_NOR_BASE + 0xAAAu, 0x00AAu);
    core->busWrite16(core, EZFLASH_S98_NOR_BASE + 0x554u, 0x0055u);
    core->busWrite16(core, EZFLASH_S98_NOR_BASE + 0xAAAu, 0x00A0u);
    core->busWrite16(core, s98_target, 0x1234u);
    if (core->busRead16(core, s98_target) != 0x1234u ||
        nor_s98.erase_commands != 1 || nor_s98.program_words != 1)
        goto done;

    /* EZ kernels DMA the outbound payload before they emit a write command. */
    cpu->gprs[0] = 0x12345678u;
    cpu->gprs[1] = 0x90ABCDEFu;
    (void)cpu->memory.storeMultiple(cpu, DATA, 0x3, LSM_IA, &transfer_cycles);
    for (unsigned i = 8; i < 512; i += 4)
        core->busWrite32(core, DATA + i, (int32_t)(0xA6B50000u | i));
    issue_command(core, 2, 0x8001u);
    if (model.blocks_read != 2 || model.blocks_written != 1 || model.failures)
        goto done;
    if (!adapter.multi_loads || !adapter.multi_stores) goto done;
    if (!adapter.synthetic_reads || !adapter.synthetic_writes) goto done;
    ezflash_mgba_sd_detach(&adapter);
    ezflash_sd_model_close(&model);

    file = fopen(path, "rb");
    if (!file || fseek(file, 2 * 512, SEEK_SET) != 0) goto done;
    uint8_t output[512];
    if (fread(output, 1, sizeof(output), file) != sizeof(output)) goto done;
    fclose(file);
    for (unsigned i = 0; i < sizeof(output); i += 4) {
        uint32_t expected = i == 0 ? 0x12345678u :
                            i == 4 ? 0x90ABCDEFu : 0xA6B50000u | i;
        if (output[i] != (uint8_t)expected ||
            output[i + 1] != (uint8_t)(expected >> 8) ||
            output[i + 2] != (uint8_t)(expected >> 16) ||
            output[i + 3] != (uint8_t)(expected >> 24)) goto done;
    }
    puts("EZ synthetic SD DMA, PSRAM pages, S71 ROM mapping, and S98 NOR writes passed");
    result = 0;
done:
    if (adapter.core) ezflash_mgba_sd_detach(&adapter);
    if (model.image) ezflash_sd_model_close(&model);
    ezflash_psram_model_deinit(&psram);
    ezflash_nor_model_deinit(&nor);
    ezflash_nor_model_deinit(&nor_s98);
    if (core) {
        if (config_initialized) mCoreConfigDeinit(&core->config);
        core->deinit(core);
    }
    unlink(path);
    if (result) fputs("EZ synthetic mGBA SD callback smoke failed\n", stderr);
    return result;
}
