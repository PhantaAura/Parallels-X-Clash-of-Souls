#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake is unavailable; running the strict Linux direct-build validation suite."
    exec "$ROOT/scripts/test-linux.sh"
fi
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/px_headless_demo
./build/px_runtime_smoke
mkdir -p review
./build/px_content_exporter review/content.json
