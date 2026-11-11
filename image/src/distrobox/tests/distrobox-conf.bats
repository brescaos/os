#!/usr/bin/env bats
# distrobox.conf, as Distrobox's commands source it: distrobox-create mounts
# profile.sh, distrobox-enter gets the colors of the distrobox it enters.

CONF=/usr/share/defaults/distrobox/distrobox.conf
COLORS=/usr/libexec/bresca/distrobox/bresca-distrobox-colors

# Sources CONF as distrobox command $1 does, with the arguments after it, and
# prints the two settings it can set.
source_as() {
    run sh -c '. "$CONF"
        printf "flags=%s\nvolumes=%s\n" "${container_manager_additional_flags:-}" "${container_additional_volumes:-}"' \
        "/usr/bin/$1" "${@:2}"
}

setup() {
    export CONF
    # The default color scheme, whoever runs the tests.
    export XDG_CONFIG_HOME=$BATS_TEST_TMPDIR/config
}

@test "distrobox-create mounts profile.sh and adds no flags" {
    source_as distrobox-create --name dev
    [[ $status -eq 0 ]]
    [[ $output == $'flags=\nvolumes=/usr/libexec/bresca/distrobox/profile.sh:/etc/profile.d/zz-brescaos-distrobox.sh:ro' ]]
}

@test "distrobox-enter gets the colors for the name it enters" {
    source_as distrobox-enter --name dev -- bash
    [[ $status -eq 0 ]]
    [[ ${lines[0]} =~ ^flags=--env=BRESCAOS_DISTROBOX_COLOR=#[0-9a-f]{6}$ ]]
    [[ ${lines[1]} =~ ^--env=BRESCAOS_DISTROBOX_LABEL_COLOR=#[0-9a-f]{6}$ ]]
    [[ ${lines[2]} == volumes= ]]
    [[ ${lines[0]#flags=}$'\n'${lines[1]} == "$("$COLORS" dev)" ]]
}

@test "other distrobox commands get nothing" {
    source_as distrobox-list
    [[ $output == $'flags=\nvolumes=' ]]
}

@test "the files are where image/Containerfile installs them" {
    [[ -x $COLORS ]]
    [[ -f /usr/libexec/bresca/distrobox/profile.sh ]]
    [[ -f $CONF ]]
}
