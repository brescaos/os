#!/usr/bin/env bash
# Pushes the image built by make image to REGISTRY as
# <REGISTRY>/<IMAGE_NAME>:<VERSION>-<ARCH> (make publish; make publish-dev for
# the local registry). Pushing a VERSION again replaces it.
#
# It never handles credentials: it uses the engine's own login (docker login /
# podman login, done beforehand).
#
# Environment: VERSION (required), REGISTRY (required: host[:port][/path]),
#              REGISTRY_TLS (required: 1 HTTPS, 0 plain HTTP), ARCH,
#              IMAGE_NAME and CONTAINER_ENGINE (defaults in settings/settings.sh).
set -euo pipefail
. "$(dirname "$0")/lib/common.sh"

require_version
[ -n "$REGISTRY" ] \
    || fail "set REGISTRY (e.g. REGISTRY=quay.io/<user>), or use 'make publish-dev' for the local registry"
case $REGISTRY in
    *://* | */) fail "REGISTRY is host[:port][/path], without a scheme or a trailing slash (got '$REGISTRY')" ;;
esac
case $REGISTRY_TLS in
    0|1) ;;
    *) fail "set REGISTRY_TLS=1 (HTTPS) or REGISTRY_TLS=0 (plain HTTP) (got '$REGISTRY_TLS')" ;;
esac
require_engine
require_local_image

remote=$REGISTRY/$IMAGE_NAME:$VERSION-$ARCH
host=${REGISTRY%%/*}
push_opts=""
case $CONTAINER_ENGINE:$REGISTRY_TLS:${host%:*} in
    # Docker trusts localhost registries over plain HTTP
    docker:0:localhost | docker:0:127.0.0.1) ;;
    docker:0:*) warn "Docker pushes over plain HTTP only to localhost, or to $host if it's in Docker's insecure-registries" ;;
    # Podman needs to be told
    podman:0:*) push_opts="--tls-verify=false" ;;
esac

step "Publishing $LOCAL_IMAGE to $remote"
engine tag "$LOCAL_IMAGE" "$remote"
# rmi even if the push fails: the extra tag is only for the push.
# shellcheck disable=SC2086 # push_opts is empty or a single flag
if ! engine push $push_opts "$remote"; then
    engine rmi "$remote" >/dev/null
    fail "push to $remote failed (logged in? run '$CONTAINER_ENGINE login $host')"
fi
engine rmi "$remote" >/dev/null
ok "published $remote"
