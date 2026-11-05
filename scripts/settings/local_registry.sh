# Settings of the local development registry (scripts/local_registry.sh) and
# how make publish-dev connects to it.
# shellcheck shell=bash
# shellcheck disable=SC2034 # used by scripts/local_registry.sh and the Makefile's publish-dev

REGISTRY_PORT=${REGISTRY_PORT:-5050}
# Docker's official registry image.
REGISTRY_IMAGE=docker.io/library/registry:3

# REGISTRY and REGISTRY_TLS for make publish-dev: it listens on localhost
# only, over plain HTTP.
LOCAL_REGISTRY=localhost:$REGISTRY_PORT
LOCAL_REGISTRY_TLS=0

# The registry's container, and the volume with its data (it survives
# restarts; make clean deletes it). Fixed: with other names, make clean
# wouldn't find the old ones.
LOCAL_REGISTRY_CONTAINER_NAME=brescaos-registry
LOCAL_REGISTRY_VOLUME_NAME=brescaos-registry
