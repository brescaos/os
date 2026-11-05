# Shared by the local registry scripts in dev/registry/ and by dev/vm/.
# shellcheck shell=bash

# shellcheck source=scripts/lib/common.sh
. "$(dirname "${BASH_SOURCE[0]}")/../../scripts/lib/common.sh"
# shellcheck source=dev/registry/settings.sh
. "$REPO_ROOT/dev/registry/settings.sh"

# True if the registry's container is running. `container inspect`, not plain
# `inspect`: the registry's data volume has the same name as its container,
# and plain `inspect` would find the volume (e.g. after `docker system prune`,
# which removes the container but not the volume).
is_registry_running() {
    [ "$(engine container inspect -f '{{.State.Running}}' "$LOCAL_REGISTRY_CONTAINER_NAME" 2>/dev/null)" = true ]
}
