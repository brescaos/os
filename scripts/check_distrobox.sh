#!/usr/bin/env bash
# Compiles the distroboxes' terminal colors (image/src/distrobox/) with their
# tests into build/distrobox/ and runs them, offscreen (make check-distrobox).
set -euo pipefail
. "$(dirname "$0")/lib/common.sh"

require_cmd cmake "Install the build dependencies in image/src/distrobox/README.md to run make check-distrobox."

build_dir=$REPO_ROOT/build/distrobox
cpu_count=$(getconf _NPROCESSORS_ONLN)

cmake -S "$REPO_ROOT/image/src/distrobox" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON >/dev/null
cmake --build "$build_dir" --parallel "$cpu_count"
ctest --test-dir "$build_dir" --output-on-failure --parallel "$cpu_count"
ok "distrobox: compiled and tested"
