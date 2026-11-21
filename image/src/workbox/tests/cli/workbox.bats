#!/usr/bin/env bats
# workbox, its own settings, its bash completion, its tray applet and its
# translations are installed, and it creates a workbox from the image its
# settings name (with the fake podman of fakes/).

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

@test "its own settings: Podman and Fedora's toolbox image" {
    grep -qx "engine=podman" /usr/share/workbox/workbox.conf
    grep -qx "image=registry.fedoraproject.org/fedora-toolbox:latest" /usr/share/workbox/workbox.conf
}

@test "create creates it from the image the user's settings name, over the others" {
    mkdir -p "$HOME/.config/workbox"
    printf '[General]\nimage=example.org/test-image:1\n' >"$HOME/.config/workbox/workbox.conf"
    run workbox create c1 --name "Cliente 01"
    [[ $status -eq 0 ]]
    [[ $output == "Created the workbox c1 (\"Cliente 01\")." ]]
    grep -qx "example.org/test-image:1" "$FAKE_STATE/calls"
}

@test "the tray applet, its QML module and its icon are installed, translated" {
    local plasmoid=/usr/share/plasma/plasmoids/com.brescaos.workbox
    [[ -f $plasmoid/contents/ui/main.qml ]]
    [[ -f $plasmoid/contents/icons/workbox-symbolic.svg ]]
    [[ -f /usr/lib64/qt6/qml/com/brescaos/workbox/qmldir ]]
    [[ -f /usr/share/icons/hicolor/scalable/apps/com.brescaos.workbox.svg ]]
    jq -e '.KPlugin.Id == "com.brescaos.workbox" and .KPlugin["Name[es]"] != null and ."X-Plasma-NotificationArea" == "true"' \
        "$plasmoid/metadata.json"
}

@test "info shows a workbox's settings" {
    printf '%s' '[{"Names": ["c1"], "State": "running", "Image": "example.org/image:1", "Labels": {"com.brescaos.workbox.name": "Cliente 01", "com.brescaos.workbox.color": "#3584e4", "com.brescaos.workbox.sudo": "true"}}]' \
        >"$FAKE_STATE/ps.json"
    run workbox info c1
    [[ $status -eq 0 ]]
    [[ $output == *"Name:    Cliente 01"* ]]
    [[ $output == *"Color:   #3584e4"* ]]
    [[ $output == *"Status:  running"* ]]
    [[ $output == *"Image:   example.org/image:1"* ]]
    [[ $output == *"Sudo:    yes"* ]]
    [[ $output == *"Home:    workbox-c1-home"* ]]
}

@test "workbox --version is VERSION.txt's" {
    run workbox --version
    [[ $output =~ ^workbox\ [0-9]+\.[0-9]+\.[0-9]+$ ]]
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
