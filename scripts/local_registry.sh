#!/usr/bin/env bash
# Local container registry for development (make publish-dev), run as a
# container on this machine. Its settings are in settings/local_registry.sh.
#
#   scripts/local_registry.sh start | stop | remove
set -euo pipefail
. "$(dirname "$0")/lib/common.sh"
# shellcheck source=scripts/settings/local_registry.sh
. "$REPO_ROOT/scripts/settings/local_registry.sh"

# `container inspect`, not plain `inspect`: the registry's data volume has the
# same name as its container, and plain `inspect` would find the volume (e.g.
# after `docker system prune`, which removes the container but not the volume).
running() {
    [ "$(engine container inspect -f '{{.State.Running}}' "$LOCAL_REGISTRY_CONTAINER_NAME" 2>/dev/null)" = true ]
}

start() {
    require_engine
    if running; then
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
    wait_ready
    ok "registry running on 127.0.0.1:$REGISTRY_PORT"
}

# Waits up to ~15 s for the registry to answer. Right after the container
# starts, Docker's port forwarder already accepts connections and closes
# them ("Empty reply from server") until the registry inside listens.
wait_ready() {
    require_cmd curl "It's needed to check that the local registry answers."
    local tries=0
    while [ "$tries" -lt 30 ]; do
        curl -fsS -o /dev/null "http://127.0.0.1:$REGISTRY_PORT/v2/" 2>/dev/null && return 0
        sleep 0.5
        tries=$((tries + 1))
    done
    fail "the local registry doesn't answer on 127.0.0.1:$REGISTRY_PORT (see '$CONTAINER_ENGINE logs $LOCAL_REGISTRY_CONTAINER_NAME')"
}

stop() {
    require_engine
    if running; then
        engine stop "$LOCAL_REGISTRY_CONTAINER_NAME" >/dev/null
    fi
    ok "registry stopped"
}

remove() {
    require_engine
    engine rm -f "$LOCAL_REGISTRY_CONTAINER_NAME" >/dev/null 2>&1 || true
    engine volume rm "$LOCAL_REGISTRY_VOLUME_NAME" >/dev/null 2>&1 || true
    ok "registry and its data removed"
}

case ${1:-} in
    start)  start ;;
    stop)   stop ;;
    remove) remove ;;
    *) fail "usage: $0 start|stop|remove" ;;
esac
