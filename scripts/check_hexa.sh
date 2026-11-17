#!/usr/bin/env bash
# Compiles Hexa's compiled parts (image/src/hexa/) with their tests into
# build/hexa/ and runs them, offscreen (make check-hexa). The decoration's
# metadata is compiled in translated, as image/Containerfile's hexa-build stage
# does.
set -euo pipefail
. "$(dirname "$0")/lib/common.sh"

require_cmd cmake "Install the build dependencies in image/src/hexa/README.md to run make check-hexa."
require_cmd msgfmt "Install gettext to run make check-hexa."
require_cmd jq "Install jq to run make check-hexa."

build_dir=$REPO_ROOT/build/hexa
cpu_count=$(getconf _NPROCESSORS_ONLN)

rm -rf "$build_dir/translations" "$build_dir/metadata"
bash "$REPO_ROOT/image/build/compile_translations.sh" "$REPO_ROOT/image/src" "$build_dir/translations" "$build_dir/metadata"
cmake -S "$REPO_ROOT/image/src/hexa" -B "$build_dir/build" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
    -DTRANSLATED_METADATA_DIR="$build_dir/metadata/hexa" >/dev/null
cmake --build "$build_dir/build" --parallel "$cpu_count"
ctest --test-dir "$build_dir/build" --output-on-failure --parallel "$cpu_count"
ok "hexa: compiled and tested"
