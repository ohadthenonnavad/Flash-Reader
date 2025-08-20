# Flash General Framework

Utilities and install steps to generate precompiled SPIBAR offsets and SPI register metadata from CHIPSEC XMLs for use by the `spi_reader` kernel module.

## Directory Layout
- `flash_general_framework/`
  - `gen_multi_spi_offsets.py` — Generate multi-platform header `kmod/include/gen_multi_spi_offsets.h`.
  - `gen_pch_spi_offsets.py` — Self-contained single-platform helpers used by the multi-generator.
  - `requirements.txt` — Optional Python dependencies (mostly stdlib; CHIPSEC optional).
  - `install.sh` — Checks prerequisites, optionally installs Python deps, and records CHIPSEC cfg root.
- `kmod/` — Linux kernel module `spi_reader` that consumes the generated header.
- `chipsec/` — Place the CHIPSEC source tree here so cfg is at `chipsec/chipsec/cfg`.

## Requirements
- Python 3 and optionally pip3 (if using `--pip-install`).
- Linux kernel headers for your running kernel (e.g., `linux-headers-$(uname -r)`).
- CHIPSEC source tree (optional; used to import a few constants if present).

## Install
```bash
# Optional: install Python deps and record CHIPSEC cfg path
./flash_general_framework/install.sh --pip-install --cfg-root /abs/path/to/chipsec/chipsec/cfg
```
If you do not pass `--cfg-root`, the build defaults to `../chipsec/chipsec/cfg` relative to `kmod/`.

## Build (multi-platform)
```bash
make -C kmod PCHS=Q170,AVN,Q370
```
This will:
- Run `flash_general_framework/gen_multi_spi_offsets.py` to generate `kmod/include/gen_multi_spi_offsets.h`.
- Build `spi_reader.ko` with `-DMULTI_PLAT` and a platform table matching the PCH list.

Override CHIPSEC cfg path if needed:
```bash
make -C kmod PCHS=Q170,AVN CFG_ROOT=/abs/path/to/chipsec/chipsec/cfg
```

## Run
```bash
sudo insmod kmod/spi_reader.ko out_path=/tmp/flash.bin offset=0 size=$((16*1024*1024))
dmesg | tail -n 100
sudo rmmod spi_reader
```

## Offset lookup helpers
Include `kmod/include/offset_lookup.h`:
- Compile-time by identifier: `u32 off = SPI_REG_OFF(fdata12);`
- Compile-time by string literal: `u32 off = SPI_REG_OFF_STR("fdata12");`
- Runtime string: `int off = spi_reg_offset_by_name("fdata12"); // -EINVAL if unknown`
