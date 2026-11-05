# BrescaOS development tasks. Run `make help` for the list.
# Variables (defaults: scripts/settings/settings.sh):
#   VERSION=1.2.5|1.2.5-rc.1 (required by image)  ARCH=aarch64|x86_64 (default: host)
#   FEDORA_VERSION, IMAGE_NAME, CONTAINER_ENGINE=docker|podman
#   NO_CACHE=1 (make image without the layer cache)

SHELL := /usr/bin/env bash
.DEFAULT_GOAL := help

.PHONY: help check image

help: ## Show this help
	@awk 'BEGIN {FS = ":.*## "} /^[a-z-]+:.*## / {printf "  \033[38;5;111m%-15s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

check: ## shellcheck + syntax on every script
	@scripts/check.sh

image: ## Build the BrescaOS image (VERSION=X.Y.Z[-alpha|beta|rc.N]; NO_CACHE=1 skips the cache)
	@scripts/image.sh build
