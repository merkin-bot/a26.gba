#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/gba/core.h>
#include <mgba/internal/gba/gba.h>

#include "supercard_mgba_bus.h"

#include <stdio.h>

int main(void) {
    uint8_t cartridge[32] = {0};
    uint32_t sdram_words[16] = {0};
    sdram_words[0] = 0x12345678u;
    SuperCardModel model = {
        .cart = cartridge,
        .cart_size = sizeof(cartridge),
        .sdram = (uint8_t *)sdram_words,
        .sdram_size = sizeof(sdram_words),
    };
    SuperCardMGBABus bus = {0};
    struct mCore *core = GBACoreCreate();
    if (!core) {
        fputs("GBACoreCreate failed\n", stderr);
        return 1;
    }
    if (!core->init(core)) {
        fputs("mGBA GBA core initialization failed\n", stderr);
        core->deinit(core);
        return 1;
    }

    mCoreInitConfig(core, NULL);
    core->reset(core);
    struct GBA *gba = core->board;
    uint32_t *firmware_rom = gba->memory.rom;
    supercard_reset(&model);
    if (!supercard_mgba_bus_attach(&bus, core, &model)) {
        fputs("SuperCard mGBA memory hook attach failed\n", stderr);
        mCoreConfigDeinit(&core->config);
        core->deinit(core);
        return 1;
    }
    core->busWrite16(core, 0x09FFFFFEu, 0xA55A);
    core->busWrite16(core, 0x09FFFFFEu, 0xA55A);
    core->busWrite16(core, 0x09FFFFFEu, 0x0007);
    if (model.mode != SUPERCARD_MODE_FIRMWARE) {
        fputs("SuperCard mode changed after only one mode write\n", stderr);
        supercard_mgba_bus_detach(&bus);
        mCoreConfigDeinit(&core->config);
        core->deinit(core);
        return 1;
    }
    core->busWrite16(core, 0x09FFFFFEu, 0x0007);
    if (model.mode != SUPERCARD_MODE_SDRAM || !model.sd_interface ||
        !model.write_access || bus.mode_register_writes != 4 ||
        bus.mode_magic_writes != 2 || bus.mode_value_writes != 2 ||
        bus.last_mode_word != 0x0007 || core->busRead16(core, 0x09800000u) != 1 ||
        core->busRead16(core, 0x09100000u) != 0x0100u ||
        gba->memory.rom != sdram_words ||
        gba->memory.romSize != sizeof(sdram_words)) {
        fputs("SuperCard mode sequence was not observed through mGBA's CPU bus\n", stderr);
        supercard_mgba_bus_detach(&bus);
        mCoreConfigDeinit(&core->config);
        core->deinit(core);
        return 1;
    }
    gba->cpu->memory.setActiveRegion(gba->cpu, 0x08000000u);
    if (gba->cpu->memory.activeRegion != sdram_words ||
        gba->cpu->memory.activeRegion[0] != 0x12345678u) {
        fputs("SuperCard SDRAM cartridge fetch mapping did not use model RAM\n",
              stderr);
        supercard_mgba_bus_detach(&bus);
        mCoreConfigDeinit(&core->config);
        core->deinit(core);
        return 1;
    }
    core->busWrite16(core, 0x09FFFFFEu, 0xA55A);
    core->busWrite16(core, 0x09FFFFFEu, 0xA55A);
    core->busWrite16(core, 0x09FFFFFEu, 0x0000);
    core->busWrite16(core, 0x09FFFFFEu, 0x0000);
    if (model.mode != SUPERCARD_MODE_FIRMWARE ||
        gba->memory.rom != firmware_rom ||
        gba->cpu->memory.activeRegion != firmware_rom) {
        fputs("SuperCard firmware mode did not restore kernel fetch mapping\n",
              stderr);
        supercard_mgba_bus_detach(&bus);
        mCoreConfigDeinit(&core->config);
        core->deinit(core);
        return 1;
    }
    supercard_mgba_bus_detach(&bus);
    mCoreConfigDeinit(&core->config);
    core->deinit(core);
    puts("mGBA static GBA core and SuperCard mode-register bus hook passed");
    return 0;
}
