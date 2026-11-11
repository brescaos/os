#!/usr/bin/env bats
# profile.sh: an interactive bash entered with colors gets its background and
# prompt, Konsole learns which distrobox it's in, and on exit the terminal
# gets its own colors back.

PROFILE=/usr/libexec/bresca/distrobox/profile.sh

setup() {
    export BRESCAOS_DISTROBOX_COLOR='#152a40' BRESCAOS_DISTROBOX_LABEL_COLOR='#a3cffd' CONTAINER_ID=dev
    unset KONSOLE_DBUS_SESSION
}

# Sources the profile in a bash with a prompt, as an interactive one, prints
# the prompt it leaves, and exits.
source_in_bash() {
    run bash -c 'PS1="\$ "; . "$1"; printf "PS1=%s\n" "$PS1"' bash "$PROFILE"
}

@test "background and text on start, the profile's back on exit" {
    source_in_bash
    [[ $status -eq 0 ]]
    [[ $output == $'\e]11;#152a40\a\e]10;#fcfcfc\aPS1='*$'\n\e]111;\a\e]110;\a' ]]
}

@test "the prompt: the box, the name in the label's color, then Fedora's" {
    source_in_bash
    [[ ${lines[0]} == *'PS1=\[\033[1;38;2;163;207;253m\]📦 dev\[\033[0m\] [\u@\h \W]\$ ' ]]
}

@test "Konsole hears of the distrobox on start and of leaving it on exit" {
    export KONSOLE_DBUS_SESSION=/Sessions/1
    source_in_bash
    [[ $output == $'\e]11;#152a40\a\e]10;#fcfcfc\a\e]777;container;push;dev;distrobox\aPS1='*$'\n\e]111;\a\e]110;\a\e]777;container;pop;;\a' ]]
}

@test "nothing changes without colors" {
    unset BRESCAOS_DISTROBOX_COLOR
    source_in_bash
    [[ $output == 'PS1=$ ' ]]
}

@test "nothing changes in a shell without a prompt" {
    run bash -c 'unset PS1; . "$1"; echo "[${PS1:-}]"' bash "$PROFILE"
    [[ $output == '[]' ]]
}

@test "it leaves no variables behind" {
    run bash -c 'PS1="\$ "; . "$1"; echo "[$(set | grep -c "^brescaos_")]"' bash "$PROFILE"
    [[ $output == *'[0]'* ]]
}
