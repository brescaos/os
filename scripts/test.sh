#!/usr/bin/env bash
# Runs the tests of every image/src/ component that has them (make test). A
# component's tests/Containerfile builds a test image on top of the BrescaOS
# image <IMAGE_NAME>:<VERSION>-<ARCH>, from its tests/ folder, and running
# that image runs the tests.
#
# Environment: VERSION (required; make test passes 0.0.0), ARCH, IMAGE_NAME
#              and CONTAINER_ENGINE (defaults in settings/settings.sh).
set -euo pipefail
. "$(dirname "$0")/lib/common.sh"

require_version
require_engine
require_local_image

failed_components=''
for containerfile in "$REPO_ROOT"/image/src/*/tests/Containerfile; do
    [ -f "$containerfile" ] || continue
    tests_dir=$(dirname "$containerfile")
    component=$(basename "$(dirname "$tests_dir")")
    test_image=$IMAGE_NAME-tests-$component:$VERSION-$ARCH

    step "Testing $component"
    engine build \
        --platform "$PLATFORM" \
        --build-arg "BASE_IMAGE=$LOCAL_IMAGE" \
        --tag "$test_image" \
        --file "$containerfile" \
        "$tests_dir"
    if engine run --rm --platform "$PLATFORM" "$test_image"; then
        ok "$component"
    else
        failed_components="$failed_components $component"
    fi
done

[ -z "$failed_components" ] || fail "tests failed:$failed_components"
