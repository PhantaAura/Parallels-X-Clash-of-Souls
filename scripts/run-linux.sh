#!/usr/bin/env bash
set -euo pipefail

PX_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [[ ! -x "${PX_ROOT}/build-linux/ParallelsX" ]]; then
  "${PX_ROOT}/scripts/build-linux.sh"
fi

export PX_LINUX_SAVE_DIR="${PX_ROOT}/dev-saves/linux"
export PX_ASSET_ROOT="${PX_ROOT}"
exec "${PX_ROOT}/build-linux/ParallelsX" "$@"
