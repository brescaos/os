#!/usr/bin/env bash
# Compiles Hexa's QML module (image/src/hexa/) with its tests into build/hexa/
# and runs them, offscreen (make check-hexa).
set -euo pipefail
. "$(dirname "$0")/lib/common.sh"

require_cmd cmake "Install the build dependencies in image/src/hexa/README.md to run make check-hexa."

build_dir=$REPO_ROOT/build/hexa
cpu_count=$(getconf _NPROCESSORS_ONLN)

cmake -S "$REPO_ROOT/image/src/hexa" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON >/dev/null
cmake --build "$build_dir" --parallel "$cpu_count"
ctest --test-dir "$build_dir" --output-on-failure --parallel "$cpu_count"
ok "hexa: compiled and tested"
