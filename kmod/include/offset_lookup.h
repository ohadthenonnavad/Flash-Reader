// SPDX-License-Identifier: GPL-2.0
#pragma once
#include <linux/types.h>
#include <linux/string.h>
#include <linux/build_bug.h>

/*
 * Helpers to retrieve SPI register offsets by name.
 * Works for both single- and multi-platform builds because g_spi_regs
 * is provided by either gen_multi_spi_offsets.h (via current platform)
 * or gen_pch_spi_offsets.h (single platform).
 */

/* Use SPI_REG_OFF(name) to get the compile-time field offset from g_spi_regs */
#define SPI_REG_OFF(name) (g_spi_regs.name)

/*
 * SPI_REG_OFF_STR(name_lit): compile-time mapping from string literal to offset.
 * Requires a string literal (enforced) and resolves via nested __builtin_choose_expr,
 * so no runtime strcmp is emitted when the argument is a constant.
 */
#define SPI_REG_OFF_STR(name_lit) \
({ \
    /* enforce literal/constant */ \
    BUILD_BUG_ON(!__builtin_constant_p(name_lit)); \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "hsfs"),  (u32)(g_spi_regs.hsfs), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "hsfc"),  (u32)(g_spi_regs.hsfc), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "faddr"), (u32)(g_spi_regs.faddr), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata0"),  (u32)(g_spi_regs.fdata0), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata1"),  (u32)(g_spi_regs.fdata1), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata2"),  (u32)(g_spi_regs.fdata2), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata3"),  (u32)(g_spi_regs.fdata3), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata4"),  (u32)(g_spi_regs.fdata4), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata5"),  (u32)(g_spi_regs.fdata5), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata6"),  (u32)(g_spi_regs.fdata6), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata7"),  (u32)(g_spi_regs.fdata7), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata8"),  (u32)(g_spi_regs.fdata8), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata9"),  (u32)(g_spi_regs.fdata9), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata10"), (u32)(g_spi_regs.fdata10), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata11"), (u32)(g_spi_regs.fdata11), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata12"), (u32)(g_spi_regs.fdata12), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata13"), (u32)(g_spi_regs.fdata13), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata14"), (u32)(g_spi_regs.fdata14), \
    __builtin_choose_expr(!__builtin_strcmp((name_lit), "fdata15"), (u32)(g_spi_regs.fdata15), \
    /* default: trigger build error for unknown name to keep it purely at compile time */ \
    ({ BUILD_BUG(); (u32)0; }) \
    ))))))))))))))))); \
})

/* X-Macro list of known SPI data/ctl registers present in the generated headers */
#define SPI_REG_FIELD_LIST(X) \
    X(hsfs) X(hsfc) X(faddr) \
    X(fdata0) X(fdata1) X(fdata2) X(fdata3) X(fdata4) X(fdata5) X(fdata6) X(fdata7) \
    X(fdata8) X(fdata9) X(fdata10) X(fdata11) X(fdata12) X(fdata13) X(fdata14) X(fdata15)

static inline int spi_reg_offset_by_name(const char *name)
{
    /* String-to-offset mapping; returns -EINVAL if not found */
    if (!name)
        return -22; /* -EINVAL */
#define CMP_NAME(nm) if (strcmp(name, #nm) == 0) return (int)g_spi_regs.nm
    SPI_REG_FIELD_LIST(CMP_NAME);
#undef CMP_NAME
    return -22; /* -EINVAL */
}
