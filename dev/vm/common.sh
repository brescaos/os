# Shared by the development VM scripts.
# shellcheck shell=bash
# shellcheck disable=SC2034 # used by the scripts that source this file

# shellcheck source=dev/registry/common.sh
. "$(dirname "${BASH_SOURCE[0]}")/../registry/common.sh"
# shellcheck source=dev/vm/settings.sh
. "$REPO_ROOT/dev/vm/settings.sh"

DISK=$VM_DIR/disk.qcow2
VARS=$VM_DIR/vars.fd

qemu_binary() {
    case $ARCH in
        x86_64)  echo qemu-system-x86_64 ;;
        aarch64) echo qemu-system-aarch64 ;;
    esac
}

require_qemu() {
    require_cmd "$(qemu_binary)" "Install QEMU (macOS: brew install qemu; Debian/Ubuntu: apt install qemu-system; Fedora: dnf install qemu)."
    require_cmd qemu-img "Install QEMU's qemu-img."
}
