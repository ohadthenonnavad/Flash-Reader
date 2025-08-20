#!/usr/bin/env python3
import argparse
import os
import sys
import xml.etree.ElementTree as ET
from typing import Optional, Tuple

# Allow importing local chipsec package if run inside repo
def add_repo_to_syspath(start_dir: str):
    # Try to find the inner chipsec package root (chipsec/chipsec)
    d = os.path.abspath(start_dir)
    while True:
        cand = os.path.join(d, 'chipsec', 'chipsec')
        if os.path.isdir(cand) and os.path.isfile(os.path.join(cand, '__init__.py')):
            if d not in sys.path:
                sys.path.insert(0, d)
            return
        parent = os.path.dirname(d)
        if parent == d:
            break
        d = parent

add_repo_to_syspath(os.getcwd())

try:
    from chipsec.hal import spi as hal_spi
except Exception as e:
    hal_spi = None


def find_sku_xml(cfg_root: str, pch_code: str) -> Optional[str]:
    """
    Locate the XML file under cfg_root containing an <sku> that matches either:
      - code == PCH_<pch_code>
      - code == <pch_code>
      - name == <pch_code>
    Searches vendor 8086 by default, with fallback to full cfg tree.
    """
    target_code_with = pch_code if pch_code.startswith('PCH_') else f'PCH_{pch_code}'
    target_code_plain = pch_code[4:] if pch_code.startswith('PCH_') else pch_code
    vendor_dir = os.path.join(cfg_root, '8086')
    if not os.path.isdir(vendor_dir):
        # fallback: search all
        vendor_dir = cfg_root
    for root, _, files in os.walk(vendor_dir):
        for fn in files:
            if not fn.endswith('.xml'):
                continue
            path = os.path.join(root, fn)
            try:
                tree = ET.parse(path)
                top = tree.getroot()
                for sku in top.findall('.//sku'):
                    code = (sku.get('code') or '').strip()
                    name = (sku.get('name') or '').strip()
                    if code in (target_code_with, target_code_plain) or name == target_code_plain:
                        return path
            except Exception:
                continue
    return None


def parse_spibar_and_regs(xml_path: str) -> Tuple[dict, dict]:
    """
    Parse SPIBAR bar definition and SPI register offsets required by the reader.
    Returns (bar_info, regs) where:
      bar_info: dictionary with keys among {bus, dev, fun, reg, width, mask, size, fixed_address}
      regs: dict of needed register offsets {HSFS, HSFC, FADDR, FDATA0..FDATA15, BIOS_PTINX, BIOS_PTDATA}
    """
    tree = ET.parse(xml_path)
    root = tree.getroot()

    # Find SPIBAR under <mmio> or <io>
    bar_info = {}
    spibar_elem = None
    for bar in root.findall('.//bar'):
        name = (bar.get('name') or '').upper()
        if name == 'SPIBAR':
            spibar_elem = bar
            # Prefer PCI BAR attributes if present
            for k in ['bus', 'dev', 'fun', 'reg', 'width', 'mask', 'size', 'fixed_address', 'offset']:
                v = bar.get(k)
                if v is None:
                    continue
                # normalize hex
                if isinstance(v, str) and v.lower().startswith('0x'):
                    try:
                        bar_info[k] = int(v, 16)
                    except ValueError:
                        bar_info[k] = v
                elif v is not None and v.isdigit():
                    bar_info[k] = int(v, 10)
                else:
                    bar_info[k] = v
            break

    # If SPIBAR references a register (e.g., register="SBASE"), derive bus/dev/fun/reg and mask
    if spibar_elem is not None and spibar_elem.get('register') and ('bus' not in bar_info or 'reg' not in bar_info):
        reg_name = (spibar_elem.get('register') or '').strip()
        base_field = (spibar_elem.get('base_field') or spibar_elem.get('base') or '').strip()
        # Find the <register name=reg_name>
        for reg in root.findall('.//register'):
            if (reg.get('name') or '').strip().upper() == reg_name.upper():
                # Pull bus/dev/fun/offset/size from register attributes
                def to_int(x):
                    if x is None:
                        return None
                    try:
                        return int(x, 0)
                    except Exception:
                        return None
                b = to_int(reg.get('bus'))
                d = to_int(reg.get('dev')) if to_int(reg.get('dev')) is not None else to_int(reg.get('device'))
                f = to_int(reg.get('fun')) if to_int(reg.get('fun')) is not None else to_int(reg.get('function'))
                off = to_int(reg.get('offset'))
                w = to_int(reg.get('size')) or 4
                if b is not None: bar_info['bus'] = b
                if d is not None: bar_info['dev'] = d
                if f is not None: bar_info['fun'] = f
                if off is not None: bar_info['reg'] = off
                bar_info.setdefault('width', w)
                # Derive mask from field if not present
                if 'mask' not in bar_info and base_field:
                    for fld in reg.findall('.//field'):
                        if (fld.get('name') or '').strip() == base_field:
                            bit = to_int(fld.get('bit')) or 0
                            size_bits = to_int(fld.get('size')) or 0
                            # Mask lower address bits below 'bit' as zero: mask = ~((1<<bit)-1)
                            if bit > 0:
                                bar_info['mask'] = (~((1 << bit) - 1)) & 0xFFFFFFFFFFFFFFFF
                            # If size provided and aligned, keep as-is; 'size' of BAR range already set from <bar>
                            break
                break

    # Registers
    need_regs = [
        'HSFS','HSFC','FADDR',
        'FDATA0','FDATA1','FDATA2','FDATA3','FDATA4','FDATA5','FDATA6','FDATA7',
        'FDATA8','FDATA9','FDATA10','FDATA11','FDATA12','FDATA13','FDATA14','FDATA15',
        'BIOS_PTINX','BIOS_PTDATA'
    ]
    regs = {k: None for k in need_regs}
    for reg in root.findall('.//register'):
        name = (reg.get('name') or '').upper()
        if name in regs:
            off = reg.get('offset')
            if off is not None:
                regs[name] = int(off, 0)

    # If some essential regs are missing (e.g., FADDR/FDATA0 live in common.xml), try to parse common.xml
    missing_now = [k for k in regs if regs[k] is None]
    if missing_now:
        cfg_vendor_dir = os.path.dirname(xml_path)
        # ascend until we hit .../cfg/8086 and use common.xml there
        while True:
            if os.path.basename(cfg_vendor_dir) == '8086':
                break
            parent = os.path.dirname(cfg_vendor_dir)
            if parent == cfg_vendor_dir:
                break
            cfg_vendor_dir = parent
        common_xml = os.path.join(cfg_vendor_dir, 'common.xml')
        if os.path.isfile(common_xml):
            try:
                ctree = ET.parse(common_xml)
                croot = ctree.getroot()
                for reg in croot.findall('.//register'):
                    name = (reg.get('name') or '').upper()
                    if name in regs and regs[name] is None:
                        off = reg.get('offset')
                        if off is not None:
                            regs[name] = int(off, 0)
            except Exception:
                pass

    # ensure minimum set present
    missing = [k for k,v in regs.items() if v is None and k in ('HSFS','HSFC','FADDR','FDATA0')]
    if missing:
        raise RuntimeError(f"Missing required SPI register offsets in {xml_path}: {missing}")

    return bar_info, regs


def emit_header(out_path: str, sku_code: str, bar: dict, regs: dict) -> None:
    """Write a C header with const structs in .rodata containing BAR metadata and register offsets and masks."""
    # Fallback constants if HAL import failed
    def const_or(name, default):
        if hal_spi is None:
            return default
        return getattr(hal_spi, name, default)

    # Correct defaults from CHIPSEC model: AEL|FCERR|FDONE = 0x07, FADDR mask 27 bits, READ cycle with GO = 0x01
    HSFSTS_CLEAR = const_or('HSFSTS_CLEAR', 0x07) or 0x07
    PCH_SCIP = const_or('PCH_RCBA_SPI_HSFSTS_SCIP', 0x01)
    PCH_FDONE = const_or('PCH_RCBA_SPI_HSFSTS_FDONE', 0x02)
    PCH_FCERR = const_or('PCH_RCBA_SPI_HSFSTS_FCERR', 0x04)
    PCH_AEL = const_or('PCH_RCBA_SPI_HSFSTS_AEL', 0x20)
    FADDR_MASK = const_or('PCH_RCBA_SPI_FADDR_MASK', 0x07FFFFFF) or 0x07FFFFFF
    HSFCTL_READ_CYCLE = const_or('HSFCTL_READ_CYCLE', 0x01) or 0x01

    lines = []
    lines.append('/* Auto-generated: DO NOT EDIT. */')
    lines.append('#pragma once')
    lines.append('#include <linux/types.h>')
    lines.append('')
    lines.append('struct spibar_meta {')
    lines.append('    u16 bus; u16 dev; u16 fun; u16 reg;')
    lines.append('    u8 width; u64 mask; u64 size; u64 fixed_address; u64 offset;')
    lines.append('};')
    lines.append('')
    lines.append('struct spi_regs {')
    for k in regs:
        lines.append(f'    u32 {k.lower()};')
    lines.append('};')
    lines.append('')
    lines.append('struct spi_consts {')
    lines.append('    u32 hsfsts_clear;')
    lines.append('    u32 hsfsts_scip; u32 hsfsts_fdone; u32 hsfsts_fcerr; u32 hsfsts_ael;')
    lines.append('    u32 faddr_mask;')
    lines.append('    u8 hsfctl_read_cycle;')
    lines.append('};')
    lines.append('')

    # Defaults for missing keys
    def getd(d, k, dv=0):
        return d[k] if k in d else dv

    lines.append('static const struct spibar_meta __ro_after_init g_spibar_meta = {')
    lines.append(f'    .bus = {getd(bar, "bus", 0)}, .dev = {getd(bar, "dev", 0)}, .fun = {getd(bar, "fun", 0)}, .reg = {getd(bar, "reg", 0)},')
    lines.append(f'    .width = {getd(bar, "width", 4)}, .mask = 0x{getd(bar, "mask", 0xFFFFFFFF):X}ULL, .size = 0x{getd(bar, "size", 0x1000):X}ULL, .fixed_address = 0x{getd(bar, "fixed_address", 0):X}ULL, .offset = 0x{getd(bar, "offset", 0):X}ULL,')
    lines.append('};')
    lines.append('')
    lines.append('static const struct spi_regs __ro_after_init g_spi_regs = {')
    for k, v in regs.items():
        if v is None:
            v = 0
        lines.append(f'    .{k.lower()} = 0x{v:X},')
    lines.append('};')
    lines.append('')
    lines.append('static const struct spi_consts __ro_after_init g_spi_consts = {')
    lines.append(f'    .hsfsts_clear = 0x{HSFSTS_CLEAR:X},')
    lines.append(f'    .hsfsts_scip = 0x{PCH_SCIP:X}, .hsfsts_fdone = 0x{PCH_FDONE:X}, .hsfsts_fcerr = 0x{PCH_FCERR:X}, .hsfsts_ael = 0x{PCH_AEL:X},')
    lines.append(f'    .faddr_mask = 0x{FADDR_MASK:X},')
    lines.append(f'    .hsfctl_read_cycle = 0x{HSFCTL_READ_CYCLE:X},')
    lines.append('};')
    lines.append('')
    lines.append(f'static const char __ro_after_init g_spi_sku[] = "{sku_code}";')

    with open(out_path, 'w') as f:
        f.write('\n'.join(lines) + '\n')


def main():
    ap = argparse.ArgumentParser(description='Generate SPIBAR/register offsets header from CHIPSEC XMLs')
    ap.add_argument('--pch', required=True, help='PCH code (e.g. Q170 or PCH_Q170)')
    ap.add_argument('--cfg-root', default=os.path.join('chipsec','chipsec','cfg'), help='Path to CHIPSEC cfg root')
    ap.add_argument('-o', '--out', required=True, help='Output header path')
    args = ap.parse_args()

    xml = find_sku_xml(args.cfg_root, args.pch)
    if not xml:
        print(f'Error: could not locate XML for PCH {args.pch} under {args.cfg_root}', file=sys.stderr)
        sys.exit(2)

    bar, regs = parse_spibar_and_regs(xml)
    sku_code = args.pch if args.pch.startswith('PCH_') else f'PCH_{args.pch}'
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    emit_header(args.out, sku_code, bar, regs)
    print(f'Generated {args.out} from {xml}')

if __name__ == '__main__':
    main()
