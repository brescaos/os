# BrescaOS settings for the host scripts. Each one can be overridden from the
# environment or the make command line (e.g. make image VERSION=0.1.0 ARCH=x86_64).
# shellcheck shell=bash
# shellcheck disable=SC2034 # used by the scripts that source lib/common.sh

# BrescaOS version to build. Required, so no default.
VERSION=${VERSION:-}

# aarch64 | x86_64. Default: the host's own architecture.
ARCH=${ARCH:-$HOST_ARCH}

# Fedora major version the image is based on.
FEDORA_VERSION=${FEDORA_VERSION:-44}

# Local image name: <IMAGE_NAME>:<VERSION>-<ARCH>.
IMAGE_NAME=${IMAGE_NAME:-brescaos}

# docker | podman.
CONTAINER_ENGINE=${CONTAINER_ENGINE:-docker}

# 1: make image builds without the layer cache, so packages come from today's
# repositories.
NO_CACHE=${NO_CACHE:-0}
