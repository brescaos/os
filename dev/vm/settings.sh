# Settings of the development VM (dev/vm/): the image installed on a QEMU
# disk without an installer. Each one can be overridden from the environment
# or the make command line (e.g. make vm VM_MEMORY=8192).
# shellcheck shell=bash
# shellcheck disable=SC2034 # used by the scripts in dev/vm/

# The VM's disk, UEFI variables and boot log; make vm-clean deletes it.
VM_DIR=$REPO_ROOT/.vm

# Memory in MiB and CPUs.
VM_MEMORY=${VM_MEMORY:-4096}
VM_CPUS=${VM_CPUS:-4}

# Size of the disk make vm creates.
DISK_SIZE=${DISK_SIZE:-20G}

# The VM's user, an administrator (wheel). Development only: the password is
# no secret.
VM_USER=${VM_USER:-bresca}
VM_FULL_NAME=${VM_FULL_NAME:-Bresca}
VM_PASSWORD=${VM_PASSWORD:-bresca}

# Keyboard as an xkb layout and variant (e.g. es / cat), time zone and
# language (a LANG value; only the languages the image has).
VM_KEYBOARD_LAYOUT=${VM_KEYBOARD_LAYOUT:-us}
VM_KEYBOARD_VARIANT=${VM_KEYBOARD_VARIANT:-}
VM_TIMEZONE=${VM_TIMEZONE:-Europe/Madrid}
VM_LANG=${VM_LANG:-en_US.UTF-8}

# The local registry as the VM reaches it: QEMU's user networking exposes the
# host as 10.0.2.2 inside the VM. The VM updates from there.
LOCAL_REGISTRY_FROM_VM=10.0.2.2:$REGISTRY_PORT

# Container volume that holds the raw disk while bootc writes it: loop
# devices need a real Linux filesystem, which a bind mount of the project
# folder isn't on macOS. make vm-clean deletes it.
VM_WORK_VOLUME=brescaos-vm-work

# UEFI firmware code and variables template. Empty: found automatically.
QEMU_FIRMWARE=${QEMU_FIRMWARE:-}
QEMU_FIRMWARE_VARS=${QEMU_FIRMWARE_VARS:-}
