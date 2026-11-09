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

# Registry make publish pushes to: host[:port][/path], e.g. quay.io/<user>
# (make publish-dev passes the local one). Required by publish, so no default.
REGISTRY=${REGISTRY:-}

# 1: REGISTRY speaks HTTPS; 0: plain HTTP. Required by publish, so no default.
REGISTRY_TLS=${REGISTRY_TLS:-}

# Languages of BrescaOS's own translations, besides English, the language of
# the code: every image/src/ component with a po/ folder has a po/<lang>.po
# for each (make check fails otherwise).
TRANSLATION_LANGUAGES=${TRANSLATION_LANGUAGES:-es ca}
