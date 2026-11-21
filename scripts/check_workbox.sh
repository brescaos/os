#!/usr/bin/env bash
# Compiles workbox's command line (image/src/workbox/) with its tests into
# build/workbox/ and runs them (make check-workbox).
set -euo pipefail
. "$(dirname "$0")/lib/common.sh"

require_cmd cmake "Install the build dependencies in image/src/workbox/README.md to run make check-workbox."

build_dir=$REPO_ROOT/build/workbox
cpu_count=$(getconf _NPROCESSORS_ONLN)

cmake -S "$REPO_ROOT/image/src/workbox" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON >/dev/null
cmake --build "$build_dir" --parallel "$cpu_count"
ctest --test-dir "$build_dir" --output-on-failure --parallel "$cpu_count"
ok "workbox: compiled and tested"
