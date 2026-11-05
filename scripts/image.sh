#!/usr/bin/env bash
# Builds the BrescaOS image <IMAGE_NAME>:<VERSION>-<ARCH> (make image).
#
# Environment: VERSION (required: X.Y.Z or X.Y.Z-<alpha|beta|rc>.N), ARCH (host),
#              FEDORA_VERSION, IMAGE_NAME, CONTAINER_ENGINE and NO_CACHE
#              (defaults in settings/settings.sh).
set -euo pipefail
. "$(dirname "$0")/lib/common.sh"

require_version
require_engine
step "Building BrescaOS $VERSION for $ARCH (Fedora $FEDORA_VERSION) with $CONTAINER_ENGINE"
[ "$ARCH" = "$HOST_ARCH" ] \
    || warn "building for $ARCH on a $HOST_ARCH host runs under emulation: expect it to be slow"

# The repository's LICENSE is outside the build context (image/): the
# image gets this copy, which is gitignored.
cp "$REPO_ROOT/LICENSE" "$REPO_ROOT/image/LICENSE"

# --pull always checks quay.io for a newer base image: when there is one,
# every BrescaOS layer is rebuilt on top of it. NO_CACHE=1 also skips the
# layer cache, so packages come from today's repositories.
cache=$(cache_opt)
[ -z "$cache" ] || step "NO_CACHE=1: building without the layer cache"
# shellcheck disable=SC2086 # cache is empty or a single flag
engine build \
    --pull $cache \
    --platform "$PLATFORM" \
    --build-arg "FEDORA_VERSION=$FEDORA_VERSION" \
    --build-arg "BRESCAOS_VERSION=$VERSION" \
    --tag "$LOCAL_IMAGE" \
    --file "$REPO_ROOT/image/Containerfile" \
    "$REPO_ROOT/image"
ok "image $LOCAL_IMAGE"
