# shellcheck shell=bash
# Where updates come from: the image this system follows, its registry and
# whether that registry is allowed without TLS. Sourced by the bresca updates
# commands. Needs root, for bootc status.

# shellcheck source=SCRIPTDIR/messages.sh
. /usr/libexec/bresca/lib/messages.sh

# The image this system follows (<registry>/<repository>:<tag>): the one
# bootc's spec names, which changes before the reboot, else the booted one.
find_followed_image() {
    bootc status --format=json | jq -r '.spec.image.image // .status.booted.image.image.image // empty'
}

# Prints the image this system follows as JSON, {image, registry,
# repository, tag, tls}: registry null when the image names none, tls false
# when its registry is allowed without TLS. Fails when bootc status shows no
# image.
print_followed_image_json() {
    local image last_component tag image_name registry='' repository has_tls=true

    image=$(find_followed_image)
    [[ -n $image ]] || return 1
    # The tag is after the last ":" that follows the last "/", so a
    # registry's port (10.0.2.2:5050) isn't taken for it. No tag means latest.
    last_component=${image##*/}
    if [[ $last_component == *:* ]]; then
        tag=${last_component##*:}
        image_name=${image%:*}
    else
        tag=latest
        image_name=$image
    fi
    repository=$image_name
    if [[ $image_name == */* ]]; then
        registry=${image_name%%/*}
        repository=${image_name#*/}
    fi
    if [[ -n $registry ]] && is_registry_allowed_without_tls "$registry"; then
        has_tls=false
    fi
    jq -cn --arg image "$image" --arg registry "$registry" --arg repository "$repository" \
        --arg tag "$tag" --argjson tls "$has_tls" \
        '{image: $image, registry: (if $registry == "" then null else $registry end),
          repository: $repository, tag: $tag, tls: $tls}'
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

warn_if_registry_without_tls() {
    local registry
    registry=$(find_registry_without_tls)
    [[ -n $registry ]] || return 0
    print_warning $"updates come from %s without TLS: anyone on the network path could serve a different image." "$registry"
}

# For the commands that download: with a registry allowed without TLS, they
# only go on once the user accepts it (--no-tls, given as true or false here).
stop_if_registry_without_tls() {
    local is_no_tls_accepted=$1
    local registry

    if $is_no_tls_accepted; then
        warn_if_registry_without_tls
        return 0
    fi
    registry=$(find_registry_without_tls)
    [[ -n $registry ]] || return 0
    print_error $"updates come from %s without TLS: anyone on the network path could serve a different image. Run it again with --no-tls to accept it." "$registry"
    exit 1
}
