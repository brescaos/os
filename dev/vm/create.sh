#!/usr/bin/env bash
# Installs brescaos:<VERSION>-<ARCH> from the local registry on a new
# .vm/disk.qcow2 (make vm, which builds the image as 0.0.0 and pushes it
# there first).
#
# The image installs itself: a short-lived privileged container of the image
# runs `bootc install to-disk` onto a loop-mounted raw file
# (dev/vm/install.sh), and the host converts that to qcow2. Privileged because
# bootc needs loop devices and their partitions.
#
# Environment: VERSION (make vm passes 0.0.0), ARCH, CONTAINER_ENGINE, VM_*
#              (defaults and meaning in dev/vm/settings.sh).
set -euo pipefail
# shellcheck source=dev/vm/common.sh
. "$(dirname "$0")/common.sh"

require_version
require_qemu
require_engine
require_local_image
is_registry_running \
    || fail "the local registry isn't running: use 'make vm', which starts it and pushes the image there"
[ "$ARCH" = "$HOST_ARCH" ] \
    || warn "installing a $ARCH image on a $HOST_ARCH host runs under emulation: slow, and may fail"

WORK=$VM_DIR/build
engine_options=""
tag=$IMAGE_NAME:$VERSION-$ARCH
rm -rf "$WORK"
mkdir -p "$WORK"
trap 'rm -rf "$WORK"' EXIT
case $ARCH in
    x86_64) serial=ttyS0 ;;
    aarch64) serial=ttyAMA0 ;;
esac
# Podman with SELinux: let the container use the host's /dev.
[ "$CONTAINER_ENGINE" = podman ] && engine_options="--security-opt label=type:unconfined_t"

step "Installing BrescaOS $VERSION ($ARCH) on a $DISK_SIZE disk: user $VM_USER, keyboard $VM_KEYBOARD_LAYOUT${VM_KEYBOARD_VARIANT:+ ($VM_KEYBOARD_VARIANT)}, time zone $VM_TIMEZONE, language $VM_LANG"
cp "$REPO_ROOT/dev/vm/install.sh" "$WORK/install.sh"
# What the container sees:
#   /dev      the engine host's devices: bootc makes a loop device for the
#             raw file and needs its partitions to show up here.
#   /var/tmp  the work volume with the raw file while it's written.
#   /io       .vm/build: install.sh goes in, the finished raw file
#             comes out.
#   network   the registry container's, so the registry is localhost:5000
#             and needs no other network or host port.
# shellcheck disable=SC2086 # engine_options is empty or one option pair
engine run --rm --privileged $engine_options \
    --platform "$PLATFORM" \
    --network "container:$LOCAL_REGISTRY_CONTAINER_NAME" \
    -v /dev:/dev \
    -v "$VM_WORK_VOLUME:/var/tmp" \
    -v "$WORK:/io" \
    -e DISK_SIZE="$DISK_SIZE" \
    -e SOURCE="localhost:5000/$tag" \
    -e TARGET="$LOCAL_REGISTRY_FROM_VM/$tag" \
    -e SERIAL="$serial" \
    -e VM_USER="$VM_USER" \
    -e VM_FULL_NAME="$VM_FULL_NAME" \
    -e VM_PASSWORD="$VM_PASSWORD" \
    -e VM_KEYBOARD_LAYOUT="$VM_KEYBOARD_LAYOUT" \
    -e VM_KEYBOARD_VARIANT="$VM_KEYBOARD_VARIANT" \
    -e VM_TIMEZONE="$VM_TIMEZONE" \
    -e VM_LANG="$VM_LANG" \
    -e HOST_UID="$(id -u)" \
    -e HOST_GID="$(id -g)" \
    "$LOCAL_IMAGE" \
    bash /io/install.sh

step "Converting to qcow2"
qemu-img convert -f raw -O qcow2 "$WORK/disk.raw" "$DISK.new"
mv "$DISK.new" "$DISK"
# Fresh disk, fresh UEFI variables.
rm -f "$VARS"
printf '%s\n' "$ARCH" >"$VM_DIR/arch"
ok "disk ready: .vm/disk.qcow2 (BrescaOS $VERSION, $ARCH)"
