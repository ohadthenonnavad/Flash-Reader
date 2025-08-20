// SPDX-License-Identifier: GPL-2.0
// Simple read-only SPI flash dumper using precompiled PCH-specific SPIBAR and register offsets
// Offsets and constants are generated from CHIPSEC XMLs into include/gen_pch_spi_offsets.h

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/version.h>

#ifdef MULTI_PLAT
#include "include/gen_multi_spi_offsets.h"
static const struct spi_platform *curp;
#define g_spibar_meta   (*curp->bar)
#define g_spi_regs      (*curp->regs)
#define g_spi_consts    (*curp->c)
#define g_spi_sku       (curp->sku)
#else
#include "include/gen_pch_spi_offsets.h"
static const void *curp; /* unused in single-platform */
#endif

MODULE_DESCRIPTION("SPI flash reader (read-only) using precompiled PCH SPIBAR offsets");
MODULE_AUTHOR("Cascade");
MODULE_LICENSE("GPL");

static char *out_path = NULL;
module_param(out_path, charp, 0444);
MODULE_PARM_DESC(out_path, "Output file path for dumped bytes");

static unsigned long long offset = 0;
module_param(offset, ullong, 0644);
MODULE_PARM_DESC(offset, "Flash offset to start reading from");

static unsigned long long size = 0;
module_param(size, ullong, 0644);
MODULE_PARM_DESC(size, "Total number of bytes to read");

static void __iomem *spi_mmio;
static resource_size_t spi_mmio_len;
static struct pci_dev *spi_pdev;

// BIOS-mapped flash fallback: last 16MB before 4GB
#define BIOS_FALLBACK_BASE   (0xFF000000ULL)
#define BIOS_FALLBACK_LEN    (0x01000000ULL) // 16MB

static void __iomem *bios_map;

static inline u8 spi_read8(u32 reg)
{
	return readb(spi_mmio + reg);
}

static inline u16 spi_read16(u32 reg)
{
	return readw(spi_mmio + reg);
}

static inline u32 spi_read32(u32 reg)
{
	return readl(spi_mmio + reg);
}

static inline void spi_write8(u32 reg, u8 val)
{
	writeb(val, spi_mmio + reg);
}

static inline void spi_write16(u32 reg, u16 val)
{
	writew(val, spi_mmio + reg);
}

static inline void spi_write32(u32 reg, u32 val)
{
	writel(val, spi_mmio + reg);
}

#ifdef MULTI_PLAT
static bool did_in_platform(const struct spi_platform *p, u16 did)
{
    u32 i;
    if (!p || !p->dids) return false;
    for (i = 0; i < p->did_count; ++i) {
        if (p->dids[i] == did)
            return true;
    }
    return false;
}

static int select_platform_by_pci(void)
{
    u32 idx;
    struct pci_dev *isa = NULL;
    u16 did = 0;
    curp = NULL;

    /* Obtain PCH Device ID from the ISA bridge (class BRIDGE_ISA, typically 00:1f.0) */
    isa = pci_get_class(PCI_CLASS_BRIDGE_ISA << 8, NULL);
    if (!isa) {
        pr_warn("[spi_reader] ISA bridge (class 0x0601) not found; cannot determine platform DID\n");
        return -ENODEV;
    }
    /* Ensure DID is host-endian explicitly */
    did = (u16)le16_to_cpu(isa->device);

    for (idx = 0; idx < g_spi_platforms_count; ++idx) {
        const struct spi_platform *p = &g_spi_platforms[idx];
        if (!did_in_platform(p, did))
            continue;

        /* Bind to the SPIBAR device at the platform-specified function (e.g., 00:1f.5) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
        spi_pdev = pci_get_domain_bus_and_slot(0, (u8)p->bar->bus, PCI_DEVFN((u8)p->bar->dev, (u8)p->bar->fun));
#else
        spi_pdev = pci_get_bus_and_slot((u8)p->bar->bus, PCI_DEVFN((u8)p->bar->dev, (u8)p->bar->fun));
#endif
        if (!spi_pdev) {
            pr_warn("[spi_reader] Platform %s matched by DID 0x%04x, but SPI device %02x:%02x.%x not present\n",
                    p->sku, did, p->bar->bus, p->bar->dev, p->bar->fun);
            continue;
        }

        curp = p;
        pr_info("[spi_reader] matched platform %s by ISA DID 0x%04x; SPIBAR at %02x:%02x.%x\n",
                p->sku, did, p->bar->bus, p->bar->dev, p->bar->fun);
        pci_dev_put(isa); /* drop ISA reference, keep spi_pdev */
        return 0;
    }

    pci_dev_put(isa);
    pr_warn("[spi_reader] No matching platform for ISA DID 0x%04x; will attempt fallback\n", did);
    return -ENODEV;
}
#endif

static bool wait_spi_cycle_done(void)
{
	u8 hsfsts;
	int i;
	for (i = 0; i < 1000; ++i) {
		hsfsts = spi_read8(g_spi_regs.hsfs);
		if (!(hsfsts & g_spi_consts.hsfsts_scip))
			break;
		udelay(100);
	}
	if (i == 1000) {
		msleep(100);
		hsfsts = spi_read8(g_spi_regs.hsfs);
		if (hsfsts & g_spi_consts.hsfsts_scip)
			return false;
	}
	// Clear FDONE/FCERR/AEL bits
	spi_write8(g_spi_regs.hsfs, (u8)g_spi_consts.hsfsts_clear);
	// Re-read and ensure no error bits set
	hsfsts = spi_read8(g_spi_regs.hsfs);
	if ((hsfsts & g_spi_consts.hsfsts_ael) || (hsfsts & g_spi_consts.hsfsts_fcerr))
		return false;
	return true;
}

static bool send_spi_read_cycle(u8 dbc_minus1, u32 fla)
{
	// Program FADDR (masked)
	spi_write32(g_spi_regs.faddr, fla & g_spi_consts.faddr_mask);
	// Program DBC (HSFC + 1)
	spi_write8(g_spi_regs.hsfc + 0x1, dbc_minus1);
	// Start cycle (HSFC with READ opcode that includes GO)
	spi_write8(g_spi_regs.hsfc, g_spi_consts.hsfctl_read_cycle);
	return wait_spi_cycle_done();
}

static int do_dump_to_file(struct file *filp, loff_t *ppos)
{
	const u32 dbc = 64; // bytes per read cycle (matches CHIPSEC default for many PCH)
	u64 remaining = size;
	u64 cur = offset;
	int ret = 0;
	u8 *buf = NULL;
	const u32 max_dwords = dbc / 4; // 16
	int i;

	buf = kmalloc(dbc, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (!wait_spi_cycle_done()) {
		ret = -EBUSY;
		goto out;
	}

	while (remaining) {
		u32 chunk = (remaining >= dbc) ? dbc : (u32)remaining;
		u32 n_dwords = DIV_ROUND_UP(chunk, 4);
		u32 j;
		if (!send_spi_read_cycle((u8)(chunk - 1), (u32)cur)) {
			ret = -EIO;
			goto out;
		}
		// Read FDATA window
		for (i = 0; i < n_dwords; ++i) {
			u32 d = spi_read32(g_spi_regs.fdata0 + i * 4);
			buf[i * 4 + 0] = (u8)((d >> 0) & 0xFF);
			buf[i * 4 + 1] = (u8)((d >> 8) & 0xFF);
			buf[i * 4 + 2] = (u8)((d >> 16) & 0xFF);
			buf[i * 4 + 3] = (u8)((d >> 24) & 0xFF);
		}
		// Write out only 'chunk' bytes
		{
			size_t to_write = chunk;
			size_t wrote = 0;
			while (wrote < to_write) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
				ssize_t n = kernel_write(filp, buf + wrote, to_write - wrote, ppos);
#else
				mm_segment_t old_fs = get_fs();
				set_fs(KERNEL_DS);
				ssize_t n = vfs_write(filp, buf + wrote, to_write - wrote, ppos);
				set_fs(old_fs);
#endif
				if (n < 0) { ret = (int)n; goto out; }
				wrote += n;
			}
		}
		cur += chunk;
		remaining -= chunk;
	}

out:
	kfree(buf);
	return ret;
}

static int do_bios_fallback_dump(struct file *filp, loff_t *ppos)
{
    u64 base = BIOS_FALLBACK_BASE;
    u64 map_len = BIOS_FALLBACK_LEN;
    u64 cur = offset;
    u64 remaining = size;
    u8 *buf = NULL;
    int ret = 0;

    if (cur >= map_len) {
        pr_err("[spi_reader] Fallback offset 0x%llx beyond BIOS window size 0x%llx\n", cur, map_len);
        return -EINVAL;
    }
    if (remaining > (map_len - cur)) {
        pr_warn("[spi_reader] Clamping size to BIOS window: requested %llu, max %llu\n", remaining, (unsigned long long)(map_len - cur));
        remaining = map_len - cur;
    }

    bios_map = ioremap((resource_size_t)base, (size_t)map_len);
    if (!bios_map) {
        pr_err("[spi_reader] ioremap BIOS window @ 0x%llx failed\n", base);
        return -ENOMEM;
    }
    pr_info("[spi_reader] Using BIOS-mapped fallback @ 0x%llx len 0x%llx\n", base, map_len);

    buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!buf) { ret = -ENOMEM; goto out_unmap; }

    while (remaining) {
        size_t chunk = (remaining > PAGE_SIZE) ? PAGE_SIZE : (size_t)remaining;
        memcpy_fromio(buf, bios_map + cur, chunk);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
        {
            ssize_t n = kernel_write(filp, buf, chunk, ppos);
            if (n < 0) { ret = (int)n; goto out_unmap; }
        }
#else
        {
            mm_segment_t old_fs = get_fs();
            set_fs(KERNEL_DS);
            ssize_t n = vfs_write(filp, buf, chunk, ppos);
            set_fs(old_fs);
            if (n < 0) { ret = (int)n; goto out_unmap; }
        }
#endif
        cur += chunk;
        remaining -= chunk;
    }

out_unmap:
    kfree(buf);
    if (bios_map) {
        iounmap(bios_map);
        bios_map = NULL;
    }
    return ret;
}

static int resolve_spibar_and_iomap(void)
{
	resource_size_t base = 0;
	resource_size_t size_len = g_spibar_meta.size ? (resource_size_t)g_spibar_meta.size : 0x1000;
	int err = 0;

	if (g_spibar_meta.fixed_address) {
		base = (resource_size_t)g_spibar_meta.fixed_address + (resource_size_t)g_spibar_meta.offset;
		goto map;
	}

	// Find PCI device by bus:dev.fun
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
	spi_pdev = pci_get_domain_bus_and_slot(0, (u8)g_spibar_meta.bus, PCI_DEVFN((u8)g_spibar_meta.dev, (u8)g_spibar_meta.fun));
#else
	spi_pdev = pci_get_bus_and_slot((u8)g_spibar_meta.bus, PCI_DEVFN((u8)g_spibar_meta.dev, (u8)g_spibar_meta.fun));
#endif
	if (!spi_pdev) {
		pr_err("[spi_reader] PCI device %02x:%02x.%x not found\n", g_spibar_meta.bus, g_spibar_meta.dev, g_spibar_meta.fun);
		return -ENODEV;
	}

	if (!pci_is_enabled(spi_pdev)) {
		err = pci_enable_device(spi_pdev);
		if (err) {
			pr_err("[spi_reader] Failed to enable PCI device: %d\n", err);
			return err;
		}
	}

	{
		u32 lo = 0, hi = 0;
		u8 width = g_spibar_meta.width ? (u8)g_spibar_meta.width : 4;
		u16 reg = g_spibar_meta.reg ? (u16)g_spibar_meta.reg : 0x10; // default BAR0
		if (width == 8) {
			pci_read_config_dword(spi_pdev, reg, &lo);
			pci_read_config_dword(spi_pdev, reg + 4, &hi);
			base = (((u64)hi) << 32) | lo;
		} else {
			pci_read_config_dword(spi_pdev, reg, &lo);
			base = lo;
		}
		if (g_spibar_meta.mask)
			base &= (resource_size_t)g_spibar_meta.mask;
		if (g_spibar_meta.offset)
			base += (resource_size_t)g_spibar_meta.offset;
	}

map:
    spi_mmio_len = size_len;
    spi_mmio = ioremap(base, spi_mmio_len);
    if (!spi_mmio) {
        pr_err("[spi_reader] ioremap failed for SPIBAR @ 0x%pa size 0x%lx\n", &base, (unsigned long)spi_mmio_len);
        return -ENOMEM;
    }
    pr_info("[spi_reader] SPIBAR mapped @ 0x%pa (len 0x%lx) for %s\n", &base, (unsigned long)spi_mmio_len, g_spi_sku);
    return 0;
}

static void unmap_and_put(void)
{
    if (spi_mmio) {
        iounmap(spi_mmio);
        spi_mmio = NULL;
    }
    if (spi_pdev) {
        pci_dev_put(spi_pdev);
        spi_pdev = NULL;
    }
}

static int __init spi_reader_init(void)
{
    struct file *filp = NULL;
    loff_t pos = 0;
    int ret;

    if (!out_path || size == 0) {
        pr_err("[spi_reader] Require module params: out_path=... size=... [offset=...]\n");
        return -EINVAL;
    }

#ifdef MULTI_PLAT
    ret = select_platform_by_pci();
    if (ret)
        goto do_bios_fallback;
#endif

    ret = resolve_spibar_and_iomap();
    if (ret) {
        pr_warn("[spi_reader] Unsupported/unknown chipset for precompiled offsets; falling back to BIOS-mapped read\n");
        goto do_bios_fallback;
    }

    // Simple sanity check: HSFS shouldn't read as 0x00 or 0xFF repeatedly
    {
        u8 h1 = spi_read8(g_spi_regs.hsfs);
        u8 h2 = spi_read8(g_spi_regs.hsfs);
        if ((h1 == 0x00 && h2 == 0x00) || (h1 == 0xFF && h2 == 0xFF)) {
            pr_warn("[spi_reader] HSFS sanity check failed (0x%02x/0x%02x); using BIOS-mapped fallback\n", h1, h2);
            unmap_and_put();
            goto do_bios_fallback;
        }
    }

    filp = filp_open(out_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (IS_ERR(filp)) {
        ret = PTR_ERR(filp);
        pr_err("[spi_reader] filp_open('%s') failed: %d\n", out_path, ret);
        goto out_unmap;
    }

    ret = do_dump_to_file(filp, &pos);
    if (ret) {
        pr_err("[spi_reader] SPIBAR dump failed: %d\n", ret);
        pr_info("[spi_reader] attempting BIOS-mapped fallback...\n");
        unmap_and_put();
        goto do_bios_fallback_after_open;
    } else {
        pr_info("[spi_reader] dump OK: wrote %llu bytes to %s\n", size, out_path);
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
    filp_close(filp, NULL);
#else
    filp_close(filp, NULL);
#endif

out_unmap:
    unmap_and_put();
out_err:
    return ret;

do_bios_fallback_after_open:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
    filp_close(filp, NULL);
#else
    filp_close(filp, NULL);
#endif
do_bios_fallback:
    {
        loff_t pos2 = 0;
        struct file *f2;
        int r2;
        f2 = filp_open(out_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (IS_ERR(f2)) {
            ret = PTR_ERR(f2);
            pr_err("[spi_reader] filp_open('%s') failed for fallback: %d\n", out_path, ret);
            goto out_err;
        }
        r2 = do_bios_fallback_dump(f2, &pos2);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
        filp_close(f2, NULL);
#else
        filp_close(f2, NULL);
#endif
        if (r2) {
            pr_err("[spi_reader] BIOS-mapped fallback failed: %d\n", r2);
            ret = r2;
            goto out_err;
        }
        pr_info("[spi_reader] fallback dump OK: wrote %llu bytes to %s\n", size, out_path);
        ret = 0;
        goto out_err;
    }
}

static void __exit spi_reader_exit(void)
{
    // Nothing to do
}

module_init(spi_reader_init);
module_exit(spi_reader_exit);
