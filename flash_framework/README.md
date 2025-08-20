# Flash Framework

Preprocessing utilities and install steps to generate precompiled SPIBAR offsets and SPI register metadata from CHIPSEC XMLs for use by the `spi_reader` kernel module.

## Directory Layout
- `flash_framework/`
  - `gen_multi_spi_offsets.py` — Generate multi-platform header `kmod/include/gen_multi_spi_offsets.h` from one or more PCH XMLs.
  - `gen_pch_spi_offsets.py` — Thin shim importing the legacy single-platform helper to parse SPIBAR/registers (used internally).
  - `install.sh` — Simple installer: check prerequisites and optionally record CHIPSEC cfg location.
- `kmod/` — Linux kernel module `spi_reader` consuming the generated headers.
- `chipsec/` — Place the CHIPSEC source tree here (so that cfg path is `chipsec/chipsec/cfg`).

## Requirements
- Python 3
- Linux kernel headers for the running kernel (e.g., `linux-headers-$(uname -r)`).
- CHIPSEC source tree available at one of:
  - `../chipsec/chipsec/cfg` relative to `kmod/Makefile` (default), or
  - A custom path provided via `CFG_ROOT=` when building, or
  - Recorded via `flash_framework/install.sh --cfg-root /path/to/chipsec/chipsec/cfg`.

## Installation
```bash
# Optional: record a custom CHIPSEC cfg path
./flash_framework/install.sh --cfg-root /abs/path/to/chipsec/chipsec/cfg
```

## Building
- Multi-platform build (recommended):
```bash
make -C kmod PCHS=Q170,AVN,Q370
```
This will:
- Run `flash_framework/gen_multi_spi_offsets.py` to generate `kmod/include/gen_multi_spi_offsets.h`.
- Compile `spi_reader.ko` with `-DMULTI_PLAT` and a platform table including all requested PCHs.

- Single-platform build (legacy, optional):
```bash
make -C kmod PCH=Q170
```

You can override the CHIPSEC cfg path:
```bash
make -C kmod PCHS=Q170,AVN CFG_ROOT=/abs/path/to/chipsec/chipsec/cfg
```

## Runtime
Insert the module and dump the SPI flash:
```bash
sudo insmod kmod/spi_reader.ko out_path=/tmp/flash.bin offset=0 size=$((16*1024*1024))
dmesg | tail -n 100  # verify platform detection and BAR mapping
sudo rmmod spi_reader
```

## Offset lookup helpers (macros)
Include `kmod/include/offset_lookup.h` to use simple helpers to access register offsets by name at compile time:
```c
#include "include/offset_lookup.h"

u32 hsfs_off = SPI_REG_OFF(hsfs);  // yields g_spi_regs.hsfs
u32 fdata7   = SPI_REG_OFF(fdata7);
```
If you need string-to-offset mapping in C (e.g., exposing a debug sysfs), use:
```c
int off = spi_reg_offset_by_name("fdata12");  // returns offset or -EINVAL
```
These work in both single- and multi-platform builds because they resolve through the same `g_spi_regs` alias.

## Where to put CHIPSEC source
Place the CHIPSEC checkout under `chipsec/` at the repository root so that the cfg directory is at `chipsec/chipsec/cfg` relative to `kmod/Makefile`. Alternatively, specify another location via `CFG_ROOT=/path/to/chipsec/chipsec/cfg` when running `make`, or record it using the installer script.
