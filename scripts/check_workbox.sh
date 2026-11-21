#!/usr/bin/env bash
# Compiles workbox (image/src/workbox/: its library, command line and tray
# applet) with its tests into build/workbox/ and runs them (make
# check-workbox). The applet's metadata is installed translated, as
# image/Containerfile's workbox-build stage does.
set -euo pipefail
. "$(dirname "$0")/lib/common.sh"

require_cmd cmake "Install the build dependencies in image/src/workbox/README.md to run make check-workbox."
require_cmd msgfmt "Install gettext to run make check-workbox."
require_cmd jq "Install jq to run make check-workbox."

build_dir=$REPO_ROOT/build/workbox
cpu_count=$(getconf _NPROCESSORS_ONLN)

rm -rf "$build_dir/translations" "$build_dir/metadata"
bash "$REPO_ROOT/image/build/compile_translations.sh" "$REPO_ROOT/image/src" "$build_dir/translations" "$build_dir/metadata"
cmake -S "$REPO_ROOT/image/src/workbox" -B "$build_dir/build" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
    -DTRANSLATED_METADATA_DIR="$build_dir/metadata/workbox" >/dev/null
cmake --build "$build_dir/build" --parallel "$cpu_count"
ctest --test-dir "$build_dir/build" --output-on-failure --parallel "$cpu_count"
ok "workbox: compiled and tested"
