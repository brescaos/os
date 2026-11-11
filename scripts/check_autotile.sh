#!/usr/bin/env bash
# Compiles autotiling (image/src/autotile/) with its tests into build/autotile/
# and runs them, offscreen (make check-autotile). Its KCM compiles in its
# translated metadata, so the translations are compiled first, as the image's
# translations stage does.
set -euo pipefail
. "$(dirname "$0")/lib/common.sh"

require_cmd cmake "Install the build dependencies in image/src/autotile/README.md to run make check-autotile."

build_dir=$REPO_ROOT/build/autotile
cpu_count=$(getconf _NPROCESSORS_ONLN)

rm -rf "$build_dir/translations" "$build_dir/metadata"
bash "$REPO_ROOT/image/build/compile_translations.sh" "$REPO_ROOT/image/src" \
    "$build_dir/translations" "$build_dir/metadata"
cmake -S "$REPO_ROOT/image/src/autotile" -B "$build_dir/build" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
    -DTRANSLATED_METADATA_DIR="$build_dir/metadata/autotile" >/dev/null
cmake --build "$build_dir/build" --parallel "$cpu_count"
ctest --test-dir "$build_dir/build" --output-on-failure --parallel "$cpu_count"
ok "autotile: compiled and tested"
