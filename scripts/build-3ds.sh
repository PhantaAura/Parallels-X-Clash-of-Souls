#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if [[ -z "${DEVKITPRO:-}" && -d /opt/devkitpro ]]; then
  export DEVKITPRO=/opt/devkitpro
fi
if [[ -n "${DEVKITPRO:-}" && -z "${DEVKITARM:-}" ]]; then
  export DEVKITARM="$DEVKITPRO/devkitARM"
fi

if [[ -z "${DEVKITPRO:-}" ]]; then
  echo "DEVKITPRO is not set."
  echo "Install devkitPro's 3DS development toolchain, then reopen Terminal."
  echo "After that, rerun: ./scripts/build-3ds.sh"
  exit 2
fi

if [[ ! -f "$DEVKITARM/3ds_rules" ]]; then
  echo "Could not find: $DEVKITARM/3ds_rules"
  echo "Your 3DS toolchain/libctru installation is incomplete."
  exit 3
fi

mkdir -p "$ROOT/src/platform/3ds/romfs/assets/characters/rrvvfo"
cp "$ROOT/assets/characters/rrvvfo/rrvvfo-dev.pxskel" "$ROOT/src/platform/3ds/romfs/assets/characters/rrvvfo/rrvvfo-dev.pxskel"
make -C "$ROOT/src/platform/3ds" clean all

echo
echo "Built: $ROOT/src/platform/3ds/parallels_x_3ds_golden_gate.3dsx"
