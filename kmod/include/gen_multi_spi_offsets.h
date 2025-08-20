/* Auto-generated: DO NOT EDIT. */
#pragma once
#include <linux/types.h>

struct spibar_meta { u16 bus; u16 dev; u16 fun; u16 reg; u8 width; u64 mask; u64 size; u64 fixed_address; u64 offset; };
struct spi_regs { u32 hsfs; u32 hsfc; u32 faddr; u32 fdata0; u32 fdata1; u32 fdata2; u32 fdata3; u32 fdata4; u32 fdata5; u32 fdata6; u32 fdata7; u32 fdata8; u32 fdata9; u32 fdata10; u32 fdata11; u32 fdata12; u32 fdata13; u32 fdata14; u32 fdata15; };
struct spi_consts { u32 hsfsts_clear; u32 hsfsts_scip; u32 hsfsts_fdone; u32 hsfsts_fcerr; u32 hsfsts_ael; u32 faddr_mask; u8 hsfctl_read_cycle; };
struct spi_platform { const struct spibar_meta *bar; const struct spi_regs *regs; const struct spi_consts *c; const char *sku; const u16 *dids; u32 did_count; };

static const struct spibar_meta __ro_after_init g_spibar_meta_0 = {
    .bus = 0,
    .dev = 31,
    .fun = 5,
    .reg = 16,
    .width = 4,
    .mask = 0xFFFFF000ULL,
    .size = 0x1000ULL,
    .fixed_address = 0x0ULL,
    .offset = 0x0ULL,
};
static const struct spi_regs __ro_after_init g_spi_regs_0 = {
    .hsfs = 0x4,
    .hsfc = 0x6,
    .faddr = 0x8,
    .fdata0 = 0x10,
    .fdata1 = 0x14,
    .fdata2 = 0x18,
    .fdata3 = 0x1C,
    .fdata4 = 0x20,
    .fdata5 = 0x24,
    .fdata6 = 0x28,
    .fdata7 = 0x2C,
    .fdata8 = 0x30,
    .fdata9 = 0x34,
    .fdata10 = 0x38,
    .fdata11 = 0x3C,
    .fdata12 = 0x40,
    .fdata13 = 0x44,
    .fdata14 = 0x48,
    .fdata15 = 0x4C,
};
static const struct spi_consts __ro_after_init g_spi_consts_0 = {
    .hsfsts_clear = 0x7,
    .hsfsts_scip = 0x1, .hsfsts_fdone = 0x2, .hsfsts_fcerr = 0x4, .hsfsts_ael = 0x20,
    .faddr_mask = 0x7FFFFFF,
    .hsfctl_read_cycle = 0x1,
};
static const u16 __ro_after_init g_spi_dids_0[] = { 0x9D43, 0x9D46, 0x9D48, 0xA143, 0xA144, 0xA145, 0xA146, 0xA147, 0xA148, 0xA149, 0xA14A, 0xA14D, 0xA14E, 0xA150, 0xA151, 0xA152, 0xA153, 0xA154, 0xA155 };
static const char __ro_after_init g_spi_sku_0[] = "PCH_Q170";

static const struct spibar_meta __ro_after_init g_spibar_meta_1 = {
    .bus = 0,
    .dev = 31,
    .fun = 0,
    .reg = 84,
    .width = 4,
    .mask = 0xFFFFFFFFFFFFFE00ULL,
    .size = 0x200ULL,
    .fixed_address = 0x0ULL,
    .offset = 0x0ULL,
};
static const struct spi_regs __ro_after_init g_spi_regs_1 = {
    .hsfs = 0x4,
    .hsfc = 0x6,
    .faddr = 0x8,
    .fdata0 = 0x10,
    .fdata1 = 0x14,
    .fdata2 = 0x18,
    .fdata3 = 0x1C,
    .fdata4 = 0x20,
    .fdata5 = 0x24,
    .fdata6 = 0x28,
    .fdata7 = 0x2C,
    .fdata8 = 0x30,
    .fdata9 = 0x34,
    .fdata10 = 0x38,
    .fdata11 = 0x3C,
    .fdata12 = 0x40,
    .fdata13 = 0x44,
    .fdata14 = 0x48,
    .fdata15 = 0x4C,
};
static const struct spi_consts __ro_after_init g_spi_consts_1 = {
    .hsfsts_clear = 0x7,
    .hsfsts_scip = 0x1, .hsfsts_fdone = 0x2, .hsfsts_fcerr = 0x4, .hsfsts_ael = 0x20,
    .faddr_mask = 0x7FFFFFF,
    .hsfctl_read_cycle = 0x1,
};
static const u16 __ro_after_init g_spi_dids_1[] = { 0x1F00, 0x1F01, 0x1F02, 0x1F03, 0x1F04, 0x1F05, 0x1F06, 0x1F07, 0x1F08, 0x1F09, 0x1F0A, 0x1F0B, 0x1F0C, 0x1F0D, 0x1F0E, 0x1F0F };
static const char __ro_after_init g_spi_sku_1[] = "PCH_AVN";

static const struct spibar_meta __ro_after_init g_spibar_meta_2 = {
    .bus = 0,
    .dev = 31,
    .fun = 5,
    .reg = 16,
    .width = 4,
    .mask = 0xFFFFF000ULL,
    .size = 0x1000ULL,
    .fixed_address = 0x0ULL,
    .offset = 0x0ULL,
};
static const struct spi_regs __ro_after_init g_spi_regs_2 = {
    .hsfs = 0x4,
    .hsfc = 0x6,
    .faddr = 0x8,
    .fdata0 = 0x10,
    .fdata1 = 0x14,
    .fdata2 = 0x18,
    .fdata3 = 0x1C,
    .fdata4 = 0x20,
    .fdata5 = 0x24,
    .fdata6 = 0x28,
    .fdata7 = 0x2C,
    .fdata8 = 0x30,
    .fdata9 = 0x34,
    .fdata10 = 0x38,
    .fdata11 = 0x3C,
    .fdata12 = 0x40,
    .fdata13 = 0x44,
    .fdata14 = 0x48,
    .fdata15 = 0x4C,
};
static const struct spi_consts __ro_after_init g_spi_consts_2 = {
    .hsfsts_clear = 0x7,
    .hsfsts_scip = 0x1, .hsfsts_fdone = 0x2, .hsfsts_fcerr = 0x4, .hsfsts_ael = 0x20,
    .faddr_mask = 0x7FFFFFF,
    .hsfctl_read_cycle = 0x1,
};
static const u16 __ro_after_init g_spi_dids_2[] = { 0xA303, 0xA304, 0xA305, 0xA306, 0xA308, 0xA309, 0xA30A, 0xA30C, 0xA30D, 0xA30E };
static const char __ro_after_init g_spi_sku_2[] = "PCH_Q370";

static const struct spi_platform __ro_after_init g_spi_platforms[] = {
    { .bar = &g_spibar_meta_0, .regs = &g_spi_regs_0, .c = &g_spi_consts_0, .sku = g_spi_sku_0, .dids = g_spi_dids_0, .did_count = (u32)(sizeof(g_spi_dids_0)/sizeof(g_spi_dids_0[0])) } ,
    { .bar = &g_spibar_meta_1, .regs = &g_spi_regs_1, .c = &g_spi_consts_1, .sku = g_spi_sku_1, .dids = g_spi_dids_1, .did_count = (u32)(sizeof(g_spi_dids_1)/sizeof(g_spi_dids_1[0])) } ,
    { .bar = &g_spibar_meta_2, .regs = &g_spi_regs_2, .c = &g_spi_consts_2, .sku = g_spi_sku_2, .dids = g_spi_dids_2, .did_count = (u32)(sizeof(g_spi_dids_2)/sizeof(g_spi_dids_2[0])) } ,
};
static const u32 __ro_after_init g_spi_platforms_count = (u32)(sizeof(g_spi_platforms)/sizeof(g_spi_platforms[0]));
