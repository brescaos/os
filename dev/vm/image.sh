#!/usr/bin/env bash
# Builds the development VM's image, <VM_IMAGE_NAME>:<VERSION>-<ARCH>, on top of
# brescaos:<VERSION>-<ARCH> (dev/vm/image/Containerfile: the shared clipboard
# and the hot reload), and pushes it to the local registry, started if needed
# (make vm, make vm-update). The VM installs it and updates from it; it's
# never published.
#
# Environment: VERSION (make passes 0.0.0), ARCH, CONTAINER_ENGINE,
#              VM_IMAGE_NAME (defaults in dev/vm/settings.sh).
set -euo pipefail
# shellcheck source=dev/vm/common.sh
. "$(dirname "$0")/common.sh"

require_version
require_engine
require_local_image

step "Building the VM's image $VM_IMAGE_NAME:$VERSION-$ARCH on $LOCAL_IMAGE"
engine build \
    --platform "$PLATFORM" \
    --build-arg "BASE_IMAGE=$LOCAL_IMAGE" \
    --tag "$VM_IMAGE_NAME:$VERSION-$ARCH" \
    --file "$REPO_ROOT/dev/vm/image/Containerfile" \
    "$REPO_ROOT/dev/vm/image"
ok "image $VM_IMAGE_NAME:$VERSION-$ARCH"

"$REPO_ROOT/dev/registry/start.sh"
IMAGE_NAME=$VM_IMAGE_NAME REGISTRY=$LOCAL_REGISTRY REGISTRY_TLS=$LOCAL_REGISTRY_TLS \
    "$REPO_ROOT/scripts/publish.sh"
