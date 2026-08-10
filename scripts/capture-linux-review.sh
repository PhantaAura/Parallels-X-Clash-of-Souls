#!/usr/bin/env bash
set -euo pipefail

PX_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PX_OUTPUT="${PX_ROOT}/review/screenshots"
mkdir -p "${PX_OUTPUT}"
"${PX_ROOT}/scripts/build-linux.sh"

PX_STATES=(
  press-start
  mode-story
  mode-arena
  mode-online
  story-character-rrvvfo
  story-character-bark
  story-character-wade
  story-character-virek
  story-so-far
  combat-manual
  chapter1-exploration
  chapter1-combat
  chapter1-dialogue
  rrvvfo-model
  rrvvfo-legacy-comparison
  rrvvfo-idle-bind
  rrvvfo-idle-sampled
  rrvvfo-idle-pose-1
  rrvvfo-idle-pose-3
  rrvvfo-idle-pose-5
  rrvvfo-idle-pose-6
)

for PX_STATE in "${PX_STATES[@]}"; do
  PX_PPM="${PX_OUTPUT}/${PX_STATE}.ppm"
  PX_PNG="${PX_OUTPUT}/${PX_STATE}.png"
  SDL_VIDEODRIVER=dummy "${PX_ROOT}/build-linux/ParallelsX" --asset-root "${PX_ROOT}" --review "${PX_STATE}" --headless --screenshot "${PX_PPM}"
  if command -v convert >/dev/null 2>&1; then
    convert "${PX_PPM}" "${PX_PNG}"
    rm "${PX_PPM}"
  fi
done

echo "Captured Linux review states in ${PX_OUTPUT}"
