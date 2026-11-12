#!/usr/bin/env bash
# Compiles the KRunner plugin (image/src/bresca-krunner/) with its tests into
# build/bresca-krunner/ and runs them, offscreen (make check-bresca-krunner). Its metadata
# is compiled in translated, as image/Containerfile's bresca-krunner-build stage does.
set -euo pipefail
. "$(dirname "$0")/lib/common.sh"

require_cmd cmake "Install the build dependencies in image/src/bresca-krunner/README.md to run make check-bresca-krunner."
require_cmd msgfmt "Install gettext to run make check-bresca-krunner."
require_cmd jq "Install jq to run make check-bresca-krunner."

build_dir=$REPO_ROOT/build/bresca-krunner
cpu_count=$(getconf _NPROCESSORS_ONLN)

rm -rf "$build_dir/translations" "$build_dir/metadata"
bash "$REPO_ROOT/image/build/compile_translations.sh" "$REPO_ROOT/image/src" "$build_dir/translations" "$build_dir/metadata"
cmake -S "$REPO_ROOT/image/src/bresca-krunner" -B "$build_dir/build" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
    -DTRANSLATED_METADATA_DIR="$build_dir/metadata/bresca-krunner" >/dev/null
cmake --build "$build_dir/build" --parallel "$cpu_count"
ctest --test-dir "$build_dir/build" --output-on-failure --parallel "$cpu_count"
ok "bresca-krunner: compiled and tested"
