#!/usr/bin/env bash
set -euo pipefail

# flash_general_framework installer
# - Verifies prerequisites (python3, kernel headers)
# - Optionally installs Python requirements
# - Optionally records CHIPSEC cfg path for Makefile usage

ROOT_DIR=$(cd "$(dirname "$0")" && pwd)
KMOD_DIR="$ROOT_DIR/../kmod"
CFG_HINT_FILE="$ROOT_DIR/.cfg_root"

usage() {
  cat >&2 <<EOF
Usage: $0 [--cfg-root /path/to/chipsec/chipsec/cfg] [--pip-install]

Options:
  --cfg-root PATH   Absolute path to CHIPSEC cfg directory (chipsec/chipsec/cfg).
  --pip-install     Run 'pip3 install -r requirements.txt' to install Python deps.

Notes:
  - CHIPSEC itself is optional. If installed, some constants are imported by the generators.
  - Default cfg path used by kmod/Makefile is ../chipsec/chipsec/cfg relative to kmod/.
EOF
}

CFG_ROOT=""
PIP_INSTALL=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --cfg-root)
      CFG_ROOT=${2-}
      shift 2
      ;;
    --pip-install)
      PIP_INSTALL=1
      shift
      ;;
    -h|--help)
      usage; exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2; usage; exit 2;
      ;;
  esac
done

# Check python3
if ! command -v python3 >/dev/null 2>&1; then
  echo "python3 not found. Please install Python 3." >&2
  exit 1
fi

# Kernel headers are required to build the module
if [[ ! -d "/lib/modules/$(uname -r)/build" ]]; then
  echo "Kernel headers for $(uname -r) not found at /lib/modules/$(uname -r)/build" >&2
  echo "Install your distro's linux-headers package." >&2
  exit 1
fi

# Optional pip install
if [[ $PIP_INSTALL -eq 1 ]]; then
  if ! command -v pip3 >/dev/null 2>&1; then
    echo "pip3 not found. Install pip for Python 3 or run without --pip-install." >&2
    exit 1
  fi
  echo "Installing Python requirements..."
  pip3 install -r "$ROOT_DIR/requirements.txt"
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
  echo "No cfg-root provided. Default remains: ../chipsec/chipsec/cfg"
fi

echo "Install completed. Build examples:"
echo "  make -C $KMOD_DIR PCHS=Q170,AVN,Q370"
echo "Override CHIPSEC cfg path:"
echo "  make -C $KMOD_DIR PCHS=Q170,AVN CFG_ROOT=/abs/path/to/chipsec/chipsec/cfg"
