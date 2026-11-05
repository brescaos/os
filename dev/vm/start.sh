#!/usr/bin/env bash
# Boots .vm/disk.qcow2 in a QEMU window (make vm, make vm-start).
# The boot log goes to .vm/serial.log.
#
# Environment: VM_MEMORY, VM_CPUS, QEMU_FIRMWARE, QEMU_FIRMWARE_VARS
#              (defaults and meaning in dev/vm/settings.sh).
set -euo pipefail
# shellcheck source=dev/vm/common.sh
. "$(dirname "$0")/common.sh"

# The VM's architecture is the disk's, whatever ARCH says now.
if [ -f "$VM_DIR/arch" ]; then
    ARCH=$(cat "$VM_DIR/arch")
fi

# Prints "<code>|<vars template or empty>" for the first firmware found.
find_firmware() {
    if [ -n "$QEMU_FIRMWARE" ]; then
        echo "$QEMU_FIRMWARE|$QEMU_FIRMWARE_VARS"
        return
    fi
    local qemu_share candidates pair
    # QEMU's own firmware (e.g. Homebrew's), next to its binary.
    qemu_share=$(cd "$(dirname "$(command -v "$(print_qemu_binary)")")/../share/qemu" 2>/dev/null && pwd || true)
    case $ARCH in
        x86_64) candidates="
            /usr/share/OVMF/OVMF_CODE_4M.fd|/usr/share/OVMF/OVMF_VARS_4M.fd
            /usr/share/edk2/ovmf/OVMF_CODE.fd|/usr/share/edk2/ovmf/OVMF_VARS.fd
            /usr/share/edk2/x64/OVMF_CODE.4m.fd|/usr/share/edk2/x64/OVMF_VARS.4m.fd
            /usr/share/OVMF/OVMF_CODE.fd|/usr/share/OVMF/OVMF_VARS.fd
            $qemu_share/edk2-x86_64-code.fd|$qemu_share/edk2-i386-vars.fd" ;;
        aarch64) candidates="
            /usr/share/AAVMF/AAVMF_CODE.fd|/usr/share/AAVMF/AAVMF_VARS.fd
            /usr/share/edk2/aarch64/QEMU_EFI-pflash.raw|/usr/share/edk2/aarch64/vars-template-pflash.raw
            /usr/share/edk2/aarch64/QEMU_CODE.fd|/usr/share/edk2/aarch64/QEMU_VARS.fd
            $qemu_share/edk2-aarch64-code.fd|$qemu_share/edk2-arm-vars.fd" ;;
    esac
    for pair in $candidates; do
        if [ -f "${pair%%|*}" ]; then
            echo "$pair"
            return
        fi
    done
    fail "no UEFI firmware found for $ARCH. Install it (Debian/Ubuntu: ovmf or qemu-efi-aarch64; Fedora: edk2-ovmf or edk2-aarch64; macOS: brew install qemu) or set QEMU_FIRMWARE"
}

# Creates the VM's UEFI variables store, .vm/vars.fd, if it isn't there.
create_uefi_vars() {
    local firmware_code=$1 firmware_vars_template=$2 size_bytes
    [ -f "$VARS" ] && return 0
    if [ -n "$firmware_vars_template" ] && [ -f "$firmware_vars_template" ]; then
        cp "$firmware_vars_template" "$VARS"
    elif [ "$ARCH" = aarch64 ]; then
        # ARM edk2 accepts an empty variable store of the same size as the code.
        size_bytes=$(wc -c < "$firmware_code" | tr -d ' ')
        qemu-img create -q -f raw "$VARS" "$size_bytes"
    else
        fail "no UEFI variables template found next to $firmware_code (set QEMU_FIRMWARE_VARS)"
    fi
    chmod u+w "$VARS"
}

# Prints QEMU's accelerator for ARCH on this host; tcg (emulation) without one.
print_qemu_accelerator() {
    if [ "$ARCH" = "$HOST_ARCH" ]; then
        case $(uname -s) in
            Darwin) echo hvf; return ;;
            Linux)  if [ -r /dev/kvm ] && [ -w /dev/kvm ]; then echo kvm; return; fi ;;
        esac
    fi
    echo tcg
}

require_qemu
[ -f "$DISK" ] || fail "no VM disk: run 'make vm' first"
accelerator=$(print_qemu_accelerator)
if [ "$accelerator" = tcg ]; then
    cpu=max
    warn "no hardware acceleration for $ARCH on this host: the VM will be slow"
else
    cpu=host
fi
case $ARCH in
    x86_64)  machine=q35 ;;
    aarch64) machine=virt ;;
esac
firmware=$(find_firmware)
firmware_code=${firmware%%|*}
create_uefi_vars "$firmware_code" "${firmware#*|}"

step "Booting .vm/disk.qcow2 ($ARCH) in a QEMU window. Boot log: .vm/serial.log; close the window or Ctrl-C to quit."
# usb-tablet: absolute pointer, the cursor follows the host's without a grab.
"$(print_qemu_binary)" \
    -machine "$machine,accel=$accelerator" \
    -cpu "$cpu" \
    -smp "$VM_CPUS" \
    -m "$VM_MEMORY" \
    -drive "if=pflash,format=raw,unit=0,readonly=on,file=$firmware_code" \
    -drive "if=pflash,format=raw,unit=1,file=$VARS" \
    -drive "file=$DISK,if=virtio,format=qcow2" \
    -netdev user,id=net0 \
    -device virtio-net-pci,netdev=net0 \
    -device virtio-rng-pci \
    -device virtio-gpu-pci \
    -device qemu-xhci \
    -device usb-kbd \
    -device usb-tablet \
    -serial "file:$VM_DIR/serial.log"
