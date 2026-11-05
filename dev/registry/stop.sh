#!/usr/bin/env bash
# Stops the local development registry; its data is kept (make registry-stop).
set -euo pipefail
# shellcheck source=dev/registry/common.sh
. "$(dirname "$0")/common.sh"

require_engine
if is_registry_running; then
    engine stop "$LOCAL_REGISTRY_CONTAINER_NAME" >/dev/null
fi
ok "registry stopped"
