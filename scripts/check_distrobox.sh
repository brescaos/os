#!/usr/bin/env bash
# Compiles the distroboxes' terminal colors, the tray applet's QML module and
# the KRunner plugin (image/src/distrobox/) with their tests into
# build/distrobox/ and runs them, offscreen (make check-distrobox). The
# plugin's metadata is compiled in translated, as image/Containerfile's
# distrobox-build stage does.
set -euo pipefail
. "$(dirname "$0")/lib/common.sh"

require_cmd cmake "Install the build dependencies in image/src/distrobox/README.md to run make check-distrobox."
require_cmd msgfmt "Install gettext to run make check-distrobox."
require_cmd jq "Install jq to run make check-distrobox."

build_dir=$REPO_ROOT/build/distrobox
cpu_count=$(getconf _NPROCESSORS_ONLN)

rm -rf "$build_dir/translations" "$build_dir/metadata"
bash "$REPO_ROOT/image/build/compile_translations.sh" "$REPO_ROOT/image/src" "$build_dir/translations" "$build_dir/metadata"
cmake -S "$REPO_ROOT/image/src/distrobox" -B "$build_dir/build" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
    -DTRANSLATED_METADATA_DIR="$build_dir/metadata/distrobox" >/dev/null
cmake --build "$build_dir/build" --parallel "$cpu_count"
ctest --test-dir "$build_dir/build" --output-on-failure --parallel "$cpu_count"
ok "distrobox: compiled and tested"
