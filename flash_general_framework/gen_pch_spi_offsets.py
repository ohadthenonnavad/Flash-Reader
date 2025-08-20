#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
"""
Self-contained single-platform helpers used by the multi-platform generator.
Provides:
  - find_sku_xml(cfg_root, pch_code) -> Optional[str]
  - parse_spibar_and_regs(xml_path) -> Tuple[dict, dict]
No dependency on legacy tools/ directory.
"""

import argparse
import os
import sys
import xml.etree.ElementTree as ET
from typing import Optional, Tuple


# Allow importing local chipsec package if run inside repo
def add_repo_to_syspath(start_dir: str):
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
    from chipsec.hal import spi as hal_spi  # type: ignore
except Exception:
    hal_spi = None  # type: ignore


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
      bar_info: dictionary with keys among {bus, dev, fun, reg, width, mask, size, fixed_address, offset}
      regs: dict of needed register offsets {HSFS, HSFC, FADDR, FDATA0..FDATA15}
    """
    tree = ET.parse(xml_path)
    root = tree.getroot()

    # Find SPIBAR under <bar name="SPIBAR">
    bar_info = {}
    spibar_elem = None
    for bar in root.findall('.//bar'):
        name = (bar.get('name') or '').upper()
        if name == 'SPIBAR':
            spibar_elem = bar
            for k in ['bus', 'dev', 'fun', 'reg', 'width', 'mask', 'size', 'fixed_address', 'offset']:
                v = bar.get(k)
                if v is None:
                    continue
                try:
                    bar_info[k] = int(v, 0)
                except Exception:
                    bar_info[k] = v
            break

    # If SPIBAR references a register (e.g., register="SBASE"), derive missing fields
    if spibar_elem is not None and spibar_elem.get('register') and ('bus' not in bar_info or 'reg' not in bar_info):
        reg_name = (spibar_elem.get('register') or '').strip()
        base_field = (spibar_elem.get('base_field') or spibar_elem.get('base') or '').strip()
        for reg in root.findall('.//register'):
            if (reg.get('name') or '').strip().upper() == reg_name.upper():
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
                if 'mask' not in bar_info and base_field:
                    for fld in reg.findall('.//field'):
                        if (fld.get('name') or '').strip() == base_field:
                            bit = to_int(fld.get('bit')) or 0
                            if bit > 0:
                                bar_info['mask'] = (~((1 << bit) - 1)) & 0xFFFFFFFFFFFFFFFF
                            break
                break

    # Registers required
    need_regs = [
        'HSFS','HSFC','FADDR',
        'FDATA0','FDATA1','FDATA2','FDATA3','FDATA4','FDATA5','FDATA6','FDATA7',
        'FDATA8','FDATA9','FDATA10','FDATA11','FDATA12','FDATA13','FDATA14','FDATA15',
    ]
    regs = {k: None for k in need_regs}
    for reg in root.findall('.//register'):
        name = (reg.get('name') or '').upper()
        if name in regs:
            off = reg.get('offset')
            if off is not None:
                regs[name] = int(off, 0)

    # If some essential regs are missing, try to parse common.xml near vendor dir
    missing_now = [k for k in regs if regs[k] is None]
    if missing_now:
        cfg_vendor_dir = os.path.dirname(xml_path)
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

    missing = [k for k,v in regs.items() if v is None and k in ('HSFS','HSFC','FADDR','FDATA0')]
    if missing:
        raise RuntimeError(f"Missing required SPI register offsets in {xml_path}: {missing}")

    # Normalize keys to lower-case names used by the multi-generator
    regs_lc = {k.lower(): (v or 0) for k, v in regs.items()}
    return bar_info, regs_lc
