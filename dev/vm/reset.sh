#!/usr/bin/env bash
# Deletes the development VM: .vm/ and the disk work volume (make vm-clean).
set -euo pipefail
# shellcheck source=dev/vm/common.sh
. "$(dirname "$0")/common.sh"

require_engine
rm -rf "$VM_DIR"
engine volume rm "$VM_WORK_VOLUME" >/dev/null 2>&1 || true
ok "removed .vm/ and the $VM_WORK_VOLUME volume"
