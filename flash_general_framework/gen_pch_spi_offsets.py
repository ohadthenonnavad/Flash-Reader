#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
# Thin shim that exposes find_sku_xml and parse_spibar_and_regs by loading the original implementation
# from the legacy tools/ directory. This avoids duplication while we complete the migration.
import importlib.util, os, sys

_here = os.path.abspath(os.path.dirname(__file__))
_legacy = os.path.abspath(os.path.join(_here, '..', 'tools', 'gen_pch_spi_offsets.py'))

if not os.path.isfile(_legacy):
    raise RuntimeError(f"Cannot locate legacy generator at: {_legacy}")

_spec = importlib.util.spec_from_file_location("gen_pch_spi_offsets_legacy", _legacy)
_mod = importlib.util.module_from_spec(_spec)
assert _spec and _spec.loader
_spec.loader.exec_module(_mod)  # type: ignore

find_sku_xml = _mod.find_sku_xml
parse_spibar_and_regs = _mod.parse_spibar_and_regs
