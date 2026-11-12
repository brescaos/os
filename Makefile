# BrescaOS development tasks. Run `make help` for the list.
# Variables (defaults: scripts/settings/settings.sh):
#   VERSION=1.2.5|1.2.5-rc.1 (required by image)  ARCH=aarch64|x86_64 (default: host)
#   FEDORA_VERSION, IMAGE_NAME, CONTAINER_ENGINE=docker|podman
#   NO_CACHE=1 (make image without the layer cache)
#   REGISTRY=host[:port][/path] REGISTRY_TLS=1|0 (required by publish)
#   REGISTRY_PORT (local registry, default 5050: dev/registry/settings.sh)
#   VM_MEMORY, VM_CPUS, DISK_SIZE, VM_USER, VM_FULL_NAME, VM_PASSWORD,
#   VM_KEYBOARD_LAYOUT, VM_KEYBOARD_VARIANT, VM_TIMEZONE, VM_LANG,
#   QEMU_AUDIO_BACKEND, QEMU_FIRMWARE, QEMU_FIRMWARE_VARS (make vm; defaults: dev/vm/settings.sh)

SHELL := /usr/bin/env bash
.DEFAULT_GOAL := help

.PHONY: help check check-scripts check-translations check-welcome check-autotile check-distrobox check-bresca-krunner format pot image test publish publish-dev registry-start registry-stop clean vm vm-update vm-start vm-clean

help: ## Show this help
	@awk 'BEGIN {FS = ":.*## "} /^[a-z-]+:.*## / {printf "  \033[38;5;111m%-19s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

# make -k check runs every one even when one fails (the CI does).
check: check-scripts check-translations check-welcome check-autotile check-distrobox check-bresca-krunner ## check-scripts, check-translations, check-welcome, check-autotile, check-distrobox and check-bresca-krunner

check-scripts: ## shellcheck, shfmt and syntax on every script
	@scripts/check.sh

check-translations: ## The translations of image/src/ are complete and in sync with the code
	@scripts/translations.sh check

check-welcome: ## Compile Bresca Welcome and run its tests (offscreen)
	@scripts/check_welcome.sh

check-autotile: ## Compile autotiling (KWin script, KCM, applet's QML module) and run its tests (offscreen)
	@scripts/check_autotile.sh

check-distrobox: ## Compile the distroboxes' terminal colors and run their tests
	@scripts/check_distrobox.sh

check-bresca-krunner: ## Compile the KRunner plugin and run its tests (offscreen)
	@scripts/check_bresca_krunner.sh

pot: ## Extract the strings of image/src/ into each po/*.pot and merge them into every .po
	@scripts/translations.sh pot

format: ## Fix the shfmt formatting of every script (style: .editorconfig)
	@shfmt -w .

image: ## Build the BrescaOS image (VERSION=X.Y.Z[-alpha|beta|rc.N]; NO_CACHE=1 skips the cache)
	@scripts/image.sh

test: ## Build the image (0.0.0) and run the tests of each image/src/ component in it
	@VERSION=0.0.0 scripts/image.sh
	@VERSION=0.0.0 scripts/test.sh

publish: ## Push the built VERSION to REGISTRY (REGISTRY_TLS=1|0) with the engine's login
	@scripts/publish.sh

publish-dev: ## make publish to the local registry (started if needed)
	@dev/registry/start.sh
	@. dev/registry/settings.sh && \
	    $(MAKE) --no-print-directory publish REGISTRY="$$LOCAL_REGISTRY" REGISTRY_TLS="$$LOCAL_REGISTRY_TLS"

registry-start: ## Start the local registry (127.0.0.1:REGISTRY_PORT)
	@dev/registry/start.sh

registry-stop: ## Stop the local registry
	@dev/registry/stop.sh

clean: ## Delete the local registry's container and its data (local brescaos:* images are kept)
	@dev/registry/remove.sh

# Development VM (dev/vm/): the image on a QEMU disk, without an installer.
# Always built as VERSION 0.0.0 and pushed only to the local registry.
vm: ## Build the image (0.0.0) and the VM's on it, push that to the local registry, install it on a new VM disk (.vm/) and boot it
	@$(MAKE) --no-print-directory image VERSION=0.0.0
	@VERSION=0.0.0 dev/vm/image.sh
	@VERSION=0.0.0 dev/vm/create.sh
	@dev/vm/start.sh

vm-update: ## Rebuild both images (0.0.0) and push the VM's as its update (in the VM: sudo bresca updates install --download --no-tls)
	@$(MAKE) --no-print-directory image VERSION=0.0.0
	@VERSION=0.0.0 dev/vm/image.sh

vm-start: ## Boot the VM disk again, as it was left
	@dev/vm/start.sh

vm-clean: ## Delete the VM (.vm/) and its disk work volume
	@dev/vm/reset.sh
