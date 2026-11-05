#!/usr/bin/env bash
# Deletes the local development registry's container and its data volume
# (make clean). Local images are kept.
set -euo pipefail
# shellcheck source=dev/registry/common.sh
. "$(dirname "$0")/common.sh"

require_engine
engine rm -f "$LOCAL_REGISTRY_CONTAINER_NAME" >/dev/null 2>&1 || true
engine volume rm "$LOCAL_REGISTRY_VOLUME_NAME" >/dev/null 2>&1 || true
ok "registry and its data removed"
