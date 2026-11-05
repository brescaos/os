# BrescaOS development tasks. Run `make help` for the list.
# Variables (defaults: scripts/settings/settings.sh):
#   VERSION=1.2.5|1.2.5-rc.1 (required by image)  ARCH=aarch64|x86_64 (default: host)
#   FEDORA_VERSION, IMAGE_NAME, CONTAINER_ENGINE=docker|podman
#   NO_CACHE=1 (make image without the layer cache)
#   REGISTRY=host[:port][/path] REGISTRY_TLS=1|0 (required by publish)
#   REGISTRY_PORT (local registry, default 5050: scripts/settings/local_registry.sh)

SHELL := /usr/bin/env bash
.DEFAULT_GOAL := help

.PHONY: help check image publish publish-dev registry-start registry-stop clean

help: ## Show this help
	@awk 'BEGIN {FS = ":.*## "} /^[a-z-]+:.*## / {printf "  \033[38;5;111m%-15s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

check: ## shellcheck + syntax on every script
	@scripts/check.sh

image: ## Build the BrescaOS image (VERSION=X.Y.Z[-alpha|beta|rc.N]; NO_CACHE=1 skips the cache)
	@scripts/image.sh

publish: ## Push the built VERSION to REGISTRY (REGISTRY_TLS=1|0) with the engine's login
	@scripts/publish.sh

publish-dev: ## make publish to the local registry (started if needed)
	@scripts/local_registry.sh start
	@. scripts/settings/local_registry.sh && \
	    $(MAKE) --no-print-directory publish REGISTRY="$$LOCAL_REGISTRY" REGISTRY_TLS="$$LOCAL_REGISTRY_TLS"

registry-start: ## Start the local registry (127.0.0.1:REGISTRY_PORT)
	@scripts/local_registry.sh start

registry-stop: ## Stop the local registry
	@scripts/local_registry.sh stop

clean: ## Delete the local registry's container and its data (local brescaos:* images are kept)
	@scripts/local_registry.sh remove
