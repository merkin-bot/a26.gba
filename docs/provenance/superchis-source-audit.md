# SuperChis source discrepancy

**Reviewed:** 2026-11-24  
**DSSMM source:** SuperFW `superfw-build769`, commit `37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4`  
**Hardware-source reference:** [ChisBread/SuperChis `superchis.vhd`](https://github.com/ChisBread/SuperChis/blob/e2b85b1533cf45d50ef1c5fafafefff200fca480/superchis.vhd), commit `e2b85b1533cf45d50ef1c5fafafefff200fca480`

The firmware source describes Chis NOR mapping and SRAM banking, but its register sequences do not match this published CPLD implementation. The harness currently records the firmware writes; it does not claim the published HDL's behavior matches every SuperChis board revision.

## Build769 firmware behavior

- `src/supercard_driver.c::push_superchis_normap` writes `0xA55A` twice, then `0x0100 | value`; `set_superchis_normap` sends eight values.
- `src/loader.c::launch_gba_nor` says it maps the game NOR blocks and sets unused entries to zero (firmware mapping).
- `src/flash_mgr.h` defines 128 MiB NOR in 4 MiB blocks; block zero is reserved and game block IDs are 1 through 31.
- `rom_boot.S` initializes the eight map entries with three writes each: magic, magic, and `0x0100`.
- `src/supercard_driver.c::sram_superchis_bank` writes the four-write mode sequence with `0x70 | ((bank & 1) << 3)`.
- `src/save.c` describes 128 KiB SRAM at `0x0E000000`, accessed as two 64 KiB banks.

These are Build769 software expectations, not a measurement of FPGA/CPLD outputs or a physical SuperChis revision.

## Published CPLD source discrepancy

At commit `e2b85b1533cf45d50ef1c5fafafefff200fca480`, `superchis.vhd` documents a four-write magic/config sequence at `0x09FFFFFE`: two magic writes followed by two configuration writes. The third write in Build769's three-write sequence is therefore only the first config write in that state machine; the VHDL does not interpret its low byte as a NOR map value. The VHDL loads five flash bank-select bits from configuration bits GP[7:3] and adds them to GP[23:21] for `FLASH_HIGH`.

For SRAM, the VHDL sets `config_sram_bank` through a write at `0x09000000` when write-enable is clear, and drives `SRAM_A16` from write-enable OR that state. Build769 instead selects the bank through mode bit 3 in `0x70`/`0x78`.

The repository describes the HDL as an open CPLD implementation/reference; it does not establish that it is identical to the CPLD revision in every SuperChis Prime. Physical behavior and the exact board revision supported by the HDL remain **UNVERIFIED**. Do not implement either interpretation as universal SuperChis hardware truth until the mismatch is resolved by revision-matched evidence or a hardware observation.

## Licensing

The SuperChis HDL repository is CC BY-NC-SA 4.0. It was consulted as a reference only; no HDL or derived source code was copied into this GPL project. Any future reuse requires a separate compatibility/license review.
