# shellcheck shell=bash
# Sourced by the bresca updates commands. Needs root, for bootc status.

# The image this system follows (<registry>/<repository>:<tag>): the one
# bootc's spec names, which changes before the reboot, else the booted one.
find_followed_image() {
    bootc status --format=json | jq -r '.spec.image.image // .status.booted.image.image.image // empty'
}

# Whether containers' registries.conf allows <registry> without TLS
# (insecure = true), e.g. the development VM's local registry.
is_registry_allowed_without_tls() {
    local registry=$1
    local config_file
    local config_files=()

    for config_file in /etc/containers/registries.conf /etc/containers/registries.conf.d/*.conf; do
        [[ -f $config_file ]] && config_files+=("$config_file")
    done
    ((${#config_files[@]} > 0)) || return 1

    # One [[registry]] table per block: its location and insecure flag are
    # checked when the next table, or the next file, starts.
    awk -v registry="$registry" '
        function check_table() {
            if (location == registry && is_insecure) found = 1
            location = ""
            is_insecure = 0
        }
        FNR == 1 || /^[[:space:]]*\[/ { check_table() }
        /^[[:space:]]*location[[:space:]]*=/ {
            value = $0
            sub(/^[^=]*=[[:space:]]*/, "", value)
            gsub(/["'\'']|[[:space:]]+$/, "", value)
            location = value
        }
        /^[[:space:]]*insecure[[:space:]]*=[[:space:]]*true/ { is_insecure = 1 }
        END { check_table(); exit !found }
    ' "${config_files[@]}"
}

# Prints the registry of the followed image if it's allowed without TLS;
# nothing otherwise.
find_registry_without_tls() {
    local image registry

    image=$(find_followed_image)
    [[ $image == */* ]] || return 0
    registry=${image%%/*}
    if is_registry_allowed_without_tls "$registry"; then
        printf '%s\n' "$registry"
    fi
}

# ⚠ and ✗ need a UTF-8 locale and a font that has them; the Linux text
# console (TERM=linux) doesn't.
can_show_unicode_symbols() {
    [[ $(locale charmap 2>/dev/null) == UTF-8 && ${TERM:-} != linux ]]
}

warn_if_registry_without_tls() {
    local registry symbol='(!)' yellow='' reset=''

    registry=$(find_registry_without_tls)
    [[ -n $registry ]] || return 0
    if can_show_unicode_symbols; then
        symbol='⚠'
    fi
    if [[ -t 2 ]]; then
        yellow=$'\e[1;33m' reset=$'\e[0m'
    fi
    printf '%s%s warning:%s updates come from %s without TLS: anyone on the network path could serve a different image.\n' \
        "$yellow" "$symbol" "$reset" "$registry" >&2
}

# For the commands that download: with a registry allowed without TLS, they
# only go on once the user accepts it (--no-tls, given as true or false here).
stop_if_registry_without_tls() {
    local is_no_tls_accepted=$1
    local registry symbol='(x)' red='' reset=''

    if $is_no_tls_accepted; then
        warn_if_registry_without_tls
        return 0
    fi

    registry=$(find_registry_without_tls)
    [[ -n $registry ]] || return 0
    if can_show_unicode_symbols; then
        symbol='✗'
    fi
    if [[ -t 2 ]]; then
        red=$'\e[1;31m' reset=$'\e[0m'
    fi
    printf '%s%s error:%s updates come from %s without TLS: anyone on the network path could serve a different image. Run it again with --no-tls to accept it.\n' \
        "$red" "$symbol" "$reset" "$registry" >&2
    exit 1
}
