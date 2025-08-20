#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
import argparse, os, sys, xml.etree.ElementTree as ET
from typing import List, Tuple, Dict, Optional

# Reuse helpers from gen_pch_spi_offsets by importing it as a module
sys.path.insert(0, os.path.dirname(__file__))
import gen_pch_spi_offsets as single


def collect_dids(xml_path: str) -> List[int]:
    try:
        tree = ET.parse(xml_path)
        root = tree.getroot()
        dids = []
        for sku in root.findall('.//sku'):
            did = sku.get('did')
            if did:
                try:
                    dids.append(int(did, 0))
                except Exception:
                    pass
        # de-dup
        dids = sorted(list({d for d in dids}))
        return dids
    except Exception:
        return []


def emit_multi_header(out_path: str, items: List[Tuple[str, dict, dict, dict, List[int]]]):
    lines: List[str] = []
    lines.append('/* Auto-generated: DO NOT EDIT. */')
    lines.append('#pragma once')
    lines.append('#include <linux/types.h>')
    lines.append('')
    lines.append('struct spibar_meta { u16 bus; u16 dev; u16 fun; u16 reg; u8 width; u64 mask; u64 size; u64 fixed_address; u64 offset; };')
    lines.append('struct spi_regs { u32 hsfs; u32 hsfc; u32 faddr; u32 fdata0; u32 fdata1; u32 fdata2; u32 fdata3; u32 fdata4; u32 fdata5; u32 fdata6; u32 fdata7; u32 fdata8; u32 fdata9; u32 fdata10; u32 fdata11; u32 fdata12; u32 fdata13; u32 fdata14; u32 fdata15; };')
    lines.append('struct spi_consts { u32 hsfsts_clear; u32 hsfsts_scip; u32 hsfsts_fdone; u32 hsfsts_fcerr; u32 hsfsts_ael; u32 faddr_mask; u8 hsfctl_read_cycle; };')
    lines.append('struct spi_platform { const struct spibar_meta *bar; const struct spi_regs *regs; const struct spi_consts *c; const char *sku; const u16 *dids; u32 did_count; };')
    lines.append('')

    # Emit per-platform constant structs and DID arrays
    for idx, (sku_code, bar, regs, consts, dids) in enumerate(items):
        lines.append(f'static const struct spibar_meta __ro_after_init g_spibar_meta_{idx} = ' + '{')
        for k in ['bus','dev','fun','reg','width','mask','size','fixed_address','offset']:
            v = bar.get(k, 0)
            if k in ('bus','dev','fun','reg','width'): lines.append(f'    .{k} = {int(v)},')
            else: lines.append(f'    .{k} = 0x{int(v):X}ULL,')
        lines.append('};')
        lines.append(f'static const struct spi_regs __ro_after_init g_spi_regs_{idx} = ' + '{')
        for k in ['hsfs','hsfc','faddr','fdata0','fdata1','fdata2','fdata3','fdata4','fdata5','fdata6','fdata7','fdata8','fdata9','fdata10','fdata11','fdata12','fdata13','fdata14','fdata15']:
            v = regs.get(k, 0) or 0
            lines.append(f'    .{k} = 0x{int(v):X},')
        lines.append('};')
        lines.append(f'static const struct spi_consts __ro_after_init g_spi_consts_{idx} = ' + '{')
        lines.append(f'    .hsfsts_clear = 0x{consts["hsfsts_clear"]:X},')
        lines.append(f'    .hsfsts_scip = 0x{consts["hsfsts_scip"]:X}, .hsfsts_fdone = 0x{consts["hsfsts_fdone"]:X}, .hsfsts_fcerr = 0x{consts["hsfsts_fcerr"]:X}, .hsfsts_ael = 0x{consts["hsfsts_ael"]:X},')
        lines.append(f'    .faddr_mask = 0x{consts["faddr_mask"]:X},')
        lines.append(f'    .hsfctl_read_cycle = 0x{consts["hsfctl_read_cycle"]:X},')
        lines.append('};')
        if not dids:
            dids = []
        arr = ', '.join(f'0x{d:04X}' for d in dids)
        lines.append(f'static const u16 __ro_after_init g_spi_dids_{idx}[] = {{ {arr} }};')
        lines.append(f'static const char __ro_after_init g_spi_sku_{idx}[] = "{sku_code}";')
        lines.append('')

    # Emit platform table
    lines.append(f'static const struct spi_platform __ro_after_init g_spi_platforms[] = ' + '{')
    for idx, _ in enumerate(items):
        lines.append('    {{ .bar = &g_spibar_meta_{0}, .regs = &g_spi_regs_{0}, .c = &g_spi_consts_{0}, .sku = g_spi_sku_{0}, .dids = g_spi_dids_{0}, .did_count = (u32)(sizeof(g_spi_dids_{0})/sizeof(g_spi_dids_{0}[0])) }} ,'.format(idx))
    lines.append('};')
    lines.append(f'static const u32 __ro_after_init g_spi_platforms_count = (u32)(sizeof(g_spi_platforms)/sizeof(g_spi_platforms[0]));')

    with open(out_path, 'w') as f:
        f.write('\n'.join(lines) + '\n')


def main():
    ap = argparse.ArgumentParser(description='Generate multi-platform SPI offsets header from CHIPSEC XMLs')
    ap.add_argument('--pchs', required=True, help='Comma-separated PCH codes (e.g. Q170,AVN or PCH_Q170,AVN)')
    ap.add_argument('--cfg-root', default=os.path.join('chipsec','chipsec','cfg'), help='Path to CHIPSEC cfg root')
    ap.add_argument('-o', '--out', required=True, help='Output header path')
    args = ap.parse_args()

    pchs = [p.strip() for p in args.pchs.split(',') if p.strip()]
    items = []
    for pch in pchs:
        xml = single.find_sku_xml(args.cfg_root, pch)
        if not xml:
            print(f'Warning: could not locate XML for {pch} under {args.cfg_root}', file=sys.stderr)
            continue
        bar, regs = single.parse_spibar_and_regs(xml)
        sku_code = pch if pch.startswith('PCH_') else f'PCH_{pch}'
        # Get constants similar to single
        # We call the internal const packer via emit_header preview by constructing dicts
        # Recompute consts here
        # Using same defaults used in emit_header
        def const_or(name: str, default: int) -> int:
            try:
                from chipsec.hal import spi as hal_spi
                v = getattr(hal_spi, name, default)
                return v if isinstance(v, int) else default
            except Exception:
                return default
        consts = {
            'hsfsts_clear': const_or('HSFSTS_CLEAR', 0x07) or 0x07,
            'hsfsts_scip': const_or('PCH_RCBA_SPI_HSFSTS_SCIP', 0x01),
            'hsfsts_fdone': const_or('PCH_RCBA_SPI_HSFSTS_FDONE', 0x02),
            'hsfsts_fcerr': const_or('PCH_RCBA_SPI_HSFSTS_FCERR', 0x04),
            'hsfsts_ael': const_or('PCH_RCBA_SPI_HSFSTS_AEL', 0x20),
            'faddr_mask': const_or('PCH_RCBA_SPI_FADDR_MASK', 0x07FFFFFF) or 0x07FFFFFF,
            'hsfctl_read_cycle': const_or('HSFCTL_READ_CYCLE', 0x01) or 0x01,
        }
        dids = collect_dids(xml)
        items.append((sku_code, bar, regs, consts, dids))

    if not items:
        print('Error: no platforms collected; nothing to emit', file=sys.stderr)
        sys.exit(2)

    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    emit_multi_header(args.out, items)
    print(f'Generated {args.out} for {len(items)} platform(s)')


if __name__ == '__main__':
    main()
