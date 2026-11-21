#!/usr/bin/env bats
# workbox, its settings and its translations are where image/Containerfile
# and build/system.sh put them, and it creates a workbox from the image they
# name (with the fake podman of fakes/).

setup() {
    export FAKE_STATE=$BATS_TEST_TMPDIR HOME=$BATS_TEST_TMPDIR LANG=C.UTF-8
    export PATH=$BATS_TEST_DIRNAME/fakes:$PATH
}

@test "workbox lists its commands" {
    run workbox --help
    [[ $status -eq 0 ]]
    [[ $output == *"create  Create a workbox"* ]]
    [[ $output == *"rm      Remove a workbox"* ]]
}

@test "its image is Distrobox's: Fedora's toolbox for this release" {
    local distrobox_image
    distrobox_image=$(sed -n 's/^container_image_default="\(.*\)"$/\1/p' /usr/share/distrobox/distrobox.conf)
    [[ $distrobox_image == registry.fedoraproject.org/fedora-toolbox:* ]]
    grep -qx "image=$distrobox_image" /usr/share/workbox/workbox.conf
    grep -qx "engine=podman" /usr/share/workbox/workbox.conf
}

@test "create creates it from that image" {
    run workbox create c1 --name "Cliente 01"
    [[ $status -eq 0 ]]
    [[ $output == "Created the workbox c1 (\"Cliente 01\")." ]]
    grep -qx "$(sed -n 's/^image=//p' /usr/share/workbox/workbox.conf)" "$FAKE_STATE/calls"
}

@test "bash completes its commands and rm the ids workbox list gives" {
    printf '%s' '[{"Names": ["c1"], "Labels": {"com.brescaos.workbox.name": "c1", "com.brescaos.workbox.color": "#3584e4"}}]' \
        >"$FAKE_STATE/ps.json"
    complete_line() {
        bash -c '. /usr/share/bash-completion/bash_completion
            . /usr/share/bash-completion/completions/workbox
            COMP_LINE=$1 COMP_POINT=${#1}
            read -ra COMP_WORDS <<<"$1"
            [[ $1 == *" " ]] && COMP_WORDS+=("")
            COMP_CWORD=$((${#COMP_WORDS[@]} - 1))
            _comp_cmd_workbox workbox "${COMP_WORDS[COMP_CWORD]}" "${COMP_WORDS[COMP_CWORD - 1]}"
            printf "%s\n" "${COMPREPLY[@]}"' _ "$1"
    }
    run complete_line "workbox "
    [[ $output == *create* && $output == *list* && $output == *rm* ]]
    run complete_line "workbox rm "
    [[ $output == c1 ]]
}

@test "it's translated" {
    local lang
    for lang in es ca; do
        [[ -f /usr/share/locale/$lang/LC_MESSAGES/workbox.mo ]] || {
            echo "missing the translations: $lang"
            return 1
        }
    done
    run env LANG=es_ES.UTF-8 workbox --help
    [[ $output == "Uso: workbox"* ]]
    run env LANG=ca_ES.UTF-8 workbox --help
    [[ $output == "Ús: workbox"* ]]
}
