#!/usr/bin/env bash
# Shared helpers for the host scripts; their settings are in scripts/settings/.
#
# Host scripts run on macOS or Linux, on arm64 or x86_64. They must stay
# compatible with bash 3.2 (macOS default): no associative arrays, no mapfile,
# no ${var,,}. Sourced, not executed.

# shellcheck disable=SC2034 # variables are used by the scripts that source this file

REPO_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)

# GitHub Actions logs show colors but are not a terminal.
if [ -t 1 ] || [ "${GITHUB_ACTIONS:-}" = true ]; then
    C_ACCENT=$'\e[38;5;111m' C_WARN=$'\e[33m' C_ERR=$'\e[31m' C_OK=$'\e[32m' C_RESET=$'\e[0m'
else
    C_ACCENT='' C_WARN='' C_ERR='' C_OK='' C_RESET=''
fi
step() { printf '%s==>%s %s\n' "$C_ACCENT" "$C_RESET" "$*"; }
ok() { printf '%s ok%s %s\n' "$C_OK" "$C_RESET" "$*"; }
warn() { printf '%swarning:%s %s\n' "$C_WARN" "$C_RESET" "$*" >&2; }
fail() {
    printf '%serror:%s %s\n' "$C_ERR" "$C_RESET" "$*" >&2
    exit 1
}

# --- Architecture -------------------------------------------------------------
# Normalised names: aarch64 | x86_64.
host_arch() {
    case $(uname -m) in
        arm64 | aarch64) echo aarch64 ;;
        x86_64 | amd64) echo x86_64 ;;
        *) fail "unsupported host architecture: $(uname -m)" ;;
    esac
}
HOST_ARCH=$(host_arch)

# --- Settings -----------------------------------------------------------------
# shellcheck source=scripts/settings/settings.sh
. "$REPO_ROOT/scripts/settings/settings.sh"

case $ARCH in
    aarch64) PLATFORM=linux/arm64 ;;
    x86_64) PLATFORM=linux/amd64 ;;
    *) fail "ARCH must be aarch64 or x86_64 (got '$ARCH')" ;;
esac

# --- Versions and names -------------------------------------------------------
LOCAL_IMAGE=$IMAGE_NAME:$VERSION-$ARCH

# BrescaOS versions: X.Y.Z or X.Y.Z-<alpha|beta|rc>.N, without leading zeros;
# N starts at 1.
VERSION_RE='^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)(-(alpha|beta|rc)\.([1-9][0-9]*))?$'

require_version() {
    [ -n "$VERSION" ] \
        || fail "set VERSION (e.g. VERSION=1.2.5, VERSION=1.2.5-rc.1, VERSION=2.1.0-alpha.1)"
    [[ $VERSION =~ $VERSION_RE ]] \
        || fail "invalid VERSION '$VERSION': use X.Y.Z or X.Y.Z-<alpha|beta|rc>.N (e.g. 1.2.5, 1.2.5-rc.1, 2.1.0-alpha.1)"
}

# --- Container engine ---------------------------------------------------------
case $CONTAINER_ENGINE in
    docker | podman) ;;
    *) fail "CONTAINER_ENGINE must be docker or podman (got '$CONTAINER_ENGINE')" ;;
esac

engine() { "$CONTAINER_ENGINE" "$@"; }

require_engine() {
    command -v "$CONTAINER_ENGINE" >/dev/null 2>&1 \
        || fail "$CONTAINER_ENGINE not found (set CONTAINER_ENGINE=docker|podman)"
    engine info >/dev/null 2>&1 \
        || fail "$CONTAINER_ENGINE is installed but not running (start Docker Desktop / the daemon)"
}

# Prints the build's cache option from NO_CACHE: nothing (0) or --no-cache (1).
print_cache_option() {
    case $NO_CACHE in
        0) ;;
        1) echo --no-cache ;;
        *) fail "NO_CACHE must be 0 or 1 (got '$NO_CACHE')" ;;
    esac
}

require_local_image() {
    engine image inspect "$LOCAL_IMAGE" >/dev/null 2>&1 \
        || fail "image $LOCAL_IMAGE not found: run 'make image VERSION=$VERSION' first"
}

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || fail "$1 not found. $2"
}
