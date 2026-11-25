#include "ezflash_mgba_sd.h"

#include <mgba/core/config.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/gba/core.h>
#include <mgba/internal/arm/arm.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
    EZFlashMGBASD adapter = {0};
    struct mCore *core = GBACoreCreate();
    int config_initialized = 0;
    int result = 1;
    if (!core || !core->init(core)) goto done;
    mCoreInitConfig(core, NULL);
    config_initialized = 1;
    core->reset(core);
    if (!ezflash_sd_model_open(&model, path, 0) ||
        !ezflash_mgba_sd_attach(&adapter, core, &model)) goto done;

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

    /* EZ kernels DMA the outbound payload before they emit a write command. */
    cpu->gprs[0] = 0x12345678u;
    cpu->gprs[1] = 0x90ABCDEFu;
    (void)cpu->memory.storeMultiple(cpu, DATA, 0x3, LSM_IA, &transfer_cycles);
    for (unsigned i = 8; i < 512; i += 4)
        core->busWrite32(core, DATA + i, (int32_t)(0xA6B50000u | i));
    issue_command(core, 2, 0x8001u);
    if (model.blocks_read != 1 || model.blocks_written != 1 || model.failures)
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
    puts("EZ synthetic SD image through mGBA bus callbacks passed read/write transcript");
    result = 0;
done:
    if (adapter.core) ezflash_mgba_sd_detach(&adapter);
    if (model.image) ezflash_sd_model_close(&model);
    if (core) {
        if (config_initialized) mCoreConfigDeinit(&core->config);
        core->deinit(core);
    }
    unlink(path);
    if (result) fputs("EZ synthetic mGBA SD callback smoke failed\n", stderr);
    return result;
}
