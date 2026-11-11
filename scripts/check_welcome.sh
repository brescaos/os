#!/usr/bin/env bash
# Compiles Bresca Welcome (image/src/welcome/) with its tests into
# build/welcome/ and runs them, offscreen (make check-welcome).
set -euo pipefail
. "$(dirname "$0")/lib/common.sh"

require_cmd cmake "Install the build dependencies in image/src/welcome/README.md to run make check-welcome."

build_dir=$REPO_ROOT/build/welcome
cpu_count=$(getconf _NPROCESSORS_ONLN)

cmake -S "$REPO_ROOT/image/src/welcome" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON >/dev/null
cmake --build "$build_dir" --parallel "$cpu_count"
ctest --test-dir "$build_dir" --output-on-failure --parallel "$cpu_count"
ok "welcome: compiled and tested"
