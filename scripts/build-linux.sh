#!/usr/bin/env bash
set -euo pipefail

PX_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PX_BUILD_DIR="${PX_ROOT}/build-linux"
PX_CXX="${CXX:-g++}"
mkdir -p "${PX_BUILD_DIR}"

PX_SHARED_SOURCES=()
while IFS= read -r PX_SOURCE; do
  PX_SHARED_SOURCES+=("${PX_SOURCE}")
done < <(find "${PX_ROOT}/src/core" "${PX_ROOT}/src/content" -name '*.cpp' -print | sort)

PX_BUILD_ARGS=(
  -std=c++17 -O2 -g -Wall -Wextra -Wpedantic -Werror
  -I"${PX_ROOT}/src"
  "${PX_SHARED_SOURCES[@]}"
  "${PX_ROOT}/src/platform/linux/main.cpp"
  "${PX_ROOT}/src/platform/desktop/sdl_application.cpp"
  "${PX_ROOT}/src/platform/desktop/presentation_renderer.cpp"
  -Wl,-l:libSDL2-2.0.so.0
  -o "${PX_BUILD_DIR}/ParallelsX"
)
"${PX_CXX}" "${PX_BUILD_ARGS[@]}"

ln -sfn ParallelsX "${PX_BUILD_DIR}/px_linux"
echo "Built ${PX_BUILD_DIR}/ParallelsX"
