#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/gba/core.h>

#include "supercard_mgba_bus.h"

#include <stdio.h>

int main(void) {
    uint8_t cartridge[32] = {0};
    uint8_t sdram[64] = {0};
    SuperCardModel model = {
        .cart = cartridge,
        .cart_size = sizeof(cartridge),
        .sdram = sdram,
        .sdram_size = sizeof(sdram),
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
        !model.write_access) {
        fputs("SuperCard mode sequence was not observed through mGBA's CPU bus\n", stderr);
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
