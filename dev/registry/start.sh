#!/usr/bin/env bash
# Starts the local development registry on 127.0.0.1:REGISTRY_PORT, a
# container from Docker's official registry image with its data in a volume
# (make registry-start; make publish-dev and make vm start it too). Does
# nothing if it's already running.
set -euo pipefail
# shellcheck source=dev/registry/common.sh
. "$(dirname "$0")/common.sh"

require_engine
if registry_running; then
    :
elif engine container inspect "$LOCAL_REGISTRY_CONTAINER_NAME" >/dev/null 2>&1; then
    engine start "$LOCAL_REGISTRY_CONTAINER_NAME" >/dev/null
else
    step "Starting the local registry on 127.0.0.1:$REGISTRY_PORT"
    engine run -d --name "$LOCAL_REGISTRY_CONTAINER_NAME" \
        -p "127.0.0.1:$REGISTRY_PORT:5000" \
        -v "$LOCAL_REGISTRY_VOLUME_NAME:/var/lib/registry" \
        "$REGISTRY_IMAGE" >/dev/null
fi

# Waits up to ~15 s for the registry to answer. Right after the container
# starts, Docker's port forwarder already accepts connections and closes
# them ("Empty reply from server") until the registry inside listens.
require_cmd curl "It's needed to check that the local registry answers."
tries=0
until curl -fsS -o /dev/null "http://127.0.0.1:$REGISTRY_PORT/v2/" 2>/dev/null; do
    [ "$tries" -lt 30 ] \
        || fail "the local registry doesn't answer on 127.0.0.1:$REGISTRY_PORT (see '$CONTAINER_ENGINE logs $LOCAL_REGISTRY_CONTAINER_NAME')"
    sleep 0.5
    tries=$((tries + 1))
done
ok "registry running on 127.0.0.1:$REGISTRY_PORT"
