#!/usr/bin/env bash
set -euo pipefail

PX_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PX_TEST_TMP="$(mktemp -d)"
trap 'rm -rf "${PX_TEST_TMP}"' EXIT

# build-linux.sh is the strict -Wall/-Wextra/-Wpedantic/-Werror platform build.
"${PX_ROOT}/scripts/build-linux.sh"

# CMake compiles px_core once and reuses it for the focused test binaries.
cmake -S "${PX_ROOT}" -B "${PX_TEST_TMP}/cmake" -DCMAKE_BUILD_TYPE=Debug >/dev/null
cmake --build "${PX_TEST_TMP}/cmake" -j2 >/dev/null
ctest --test-dir "${PX_TEST_TMP}/cmake" --output-on-failure

python3 "${PX_ROOT}/scripts/author-rrvvfo-idle.py" \
  "${PX_ROOT}/assets/characters/rrvvfo/rrvvfo-dev.glb" \
  "${PX_TEST_TMP}/rrvvfo-idle.animation.json"
cmp "${PX_TEST_TMP}/rrvvfo-idle.animation.json" "${PX_ROOT}/assets/characters/rrvvfo/rrvvfo-idle.animation.json"
python3 "${PX_ROOT}/scripts/author-rrvvfo-core-animations.py" \
  "${PX_ROOT}/assets/characters/rrvvfo/rrvvfo-dev.glb" \
  "${PX_ROOT}/assets/characters/rrvvfo/rrvvfo-idle.animation.json" \
  "${PX_TEST_TMP}/rrvvfo-core.animation.json"
cmp "${PX_TEST_TMP}/rrvvfo-core.animation.json" "${PX_ROOT}/assets/characters/rrvvfo/rrvvfo-core.animation.json"
python3 "${PX_ROOT}/scripts/cook-character-glb.py" \
  "${PX_ROOT}/assets/characters/rrvvfo/rrvvfo-dev.glb" \
  "${PX_TEST_TMP}/rrvvfo-dev.pxskel" \
  --settings "${PX_ROOT}/assets/characters/rrvvfo/rrvvfo-dev.materials.json" \
  --animations "${PX_ROOT}/assets/characters/rrvvfo/rrvvfo-core.animation.json"
cmp "${PX_TEST_TMP}/rrvvfo-dev.pxskel" "${PX_ROOT}/assets/characters/rrvvfo/rrvvfo-dev.pxskel"
cmp "${PX_ROOT}/assets/characters/rrvvfo/rrvvfo-dev.pxskel" \
    "${PX_ROOT}/src/platform/3ds/romfs/assets/characters/rrvvfo/rrvvfo-dev.pxskel"
python3 "${PX_ROOT}/tests/source_architecture_tests.py"
"${PX_TEST_TMP}/cmake/px_content_exporter" "${PX_TEST_TMP}/content.json"
python3 -m json.tool "${PX_TEST_TMP}/content.json" >/dev/null

PX_REVIEW_STATES=(
  press-start mode-story mode-arena mode-online story-character-rrvvfo
  story-so-far combat-manual chapter1-exploration chapter1-combat chapter1-dialogue
  rrvvfo-model rrvvfo-legacy-comparison rrvvfo-idle-bind rrvvfo-idle-sampled
  rrvvfo-idle-pose-1 rrvvfo-idle-pose-3 rrvvfo-idle-pose-5 rrvvfo-idle-pose-6
  rrvvfo-run rrvvfo-dash rrvvfo-charge rrvvfo-heavy rrvvfo-fire-blast rrvvfo-object-swap rrvvfo-lens
)
for PX_STATE in "${PX_REVIEW_STATES[@]}"; do
  SDL_VIDEODRIVER=dummy "${PX_ROOT}/build-linux/ParallelsX" --asset-root "${PX_ROOT}" --review "${PX_STATE}" --headless --screenshot "${PX_TEST_TMP}/${PX_STATE}.ppm"
  test -s "${PX_TEST_TMP}/${PX_STATE}.ppm"
done

echo "PASS: strict Linux build, 5 focused suites, deterministic Rrvvfo cook, 3DS ROMFS parity, and ${#PX_REVIEW_STATES[@]} graphical review states"
