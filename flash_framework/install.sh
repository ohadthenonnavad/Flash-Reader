#!/usr/bin/env bash
set -euo pipefail

# flash_framework installer
# - Verifies prerequisites
# - Records CHIPSEC cfg path if provided
# - Prints usage instructions

ROOT_DIR=$(cd "$(dirname "$0")" && pwd)
KMOD_DIR="$ROOT_DIR/../kmod"
CFG_HINT_FILE="$ROOT_DIR/.cfg_root"

usage() {
  echo "Usage: $0 [--cfg-root /path/to/chipsec/chipsec/cfg]" >&2
}

CFG_ROOT=""
if [[ ${1-} == "--cfg-root" ]]; then
  CFG_ROOT=${2-}
  if [[ -z "$CFG_ROOT" ]]; then usage; exit 2; fi
fi

# Check python3
if ! command -v python3 >/dev/null 2>&1; then
  echo "python3 not found. Please install Python 3." >&2
  exit 1
fi

# Check kernel headers
if [[ ! -d "/lib/modules/$(uname -r)/build" ]]; then
  echo "Kernel headers for $(uname -r) not found at /lib/modules/$(uname -r)/build" >&2
  echo "Install your distro's linux-headers package." >&2
  exit 1
fi

# Save cfg root hint if provided
if [[ -n "$CFG_ROOT" ]]; then
  if [[ ! -d "$CFG_ROOT" ]]; then
    echo "Given cfg-root does not exist: $CFG_ROOT" >&2
    exit 1
  fi
  echo "$CFG_ROOT" > "$CFG_HINT_FILE"
  echo "Recorded CHIPSEC cfg root: $CFG_ROOT"
else
  echo "No cfg-root provided. Using default relative to repo: ../chipsec/chipsec/cfg"
fi

echo "Install completed. Build examples:"
echo "  make -C $KMOD_DIR PCHS=Q170,AVN,Q370"
echo "  sudo insmod $KMOD_DIR/spi_reader.ko out_path=/tmp/flash.bin offset=0 size=$((16*1024*1024))"
