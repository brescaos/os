# shellcheck shell=bash
# Loaded by every test file: a fresh fake system for each test (fakes/bootc,
# fakes/systemctl) and helpers to describe it.

# run --separate-stderr.
bats_require_minimum_version 1.5.0

# The image the fake system follows. Nothing is pulled: it's only a name.
IMAGE=registry.brescaos.com/brescaos/brescaos:0.1.0-x86_64

setup() {
    export BOOTC_STATE=$BATS_TEST_TMPDIR/bootc-state.json
    export SYSTEMCTL_LOG=$BATS_TEST_TMPDIR/systemctl.log
    unset BOOTC_REGISTRY_VERSION BOOTC_FAILS
    : >"$SYSTEMCTL_LOG"
    rm -rf /run/bresca/updates /etc/containers/registries.conf.d/50-tests.conf
    boot_version 0.1.0
}

teardown() {
    rm -rf /run/bresca/updates /etc/containers/registries.conf.d/50-tests.conf
}

# The system boots version $1 of IMAGE (or of $2): nothing staged, no rollback.
boot_version() {
    local image=${2:-$IMAGE}
    jq -n --arg version "$1" --arg image "$image" '
        {spec: {image: {image: $image}},
         status: {booted: {image: {image: {image: $image}, version: $version,
                                   imageDigest: ("sha256:" + $version)}},
                  staged: null, rollback: null, rollbackQueued: false}}' >"$BOOTC_STATE"
}

change_state() {
    jq "$@" "$BOOTC_STATE" >"$BOOTC_STATE.new"
    mv "$BOOTC_STATE.new" "$BOOTC_STATE"
}

# Version $1 is staged: downloaded ($2 = downloaded) or installed.
stage_version() {
    change_state --arg version "$1" --argjson download_only "$([[ $2 == downloaded ]] && echo true || echo false)" '
        .status.staged = {image: {image: {image: .spec.image.image}, version: $version},
                          downloadOnly: $download_only}'
}

keep_rollback_version() {
    change_state --arg version "$1" '.status.rollback = {image: {image: {image: .spec.image.image}, version: $version}}'
}

# The registry has version $1.
publish_version() {
    export BOOTC_REGISTRY_VERSION=$1
}

# containers' registries.conf allows registry $1 without TLS.
allow_without_tls() {
    mkdir -p /etc/containers/registries.conf.d
    printf '[[registry]]\nlocation = "%s"\ninsecure = true\n' "$1" >/etc/containers/registries.conf.d/50-tests.conf
}

staged_field() {
    jq -r ".status.staged.$1" "$BOOTC_STATE"
}
