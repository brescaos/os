#!/usr/bin/env bats
# The program, its .desktop files and its translations are where
# image/Containerfile installs them, and it runs in the image (offscreen).

PROGRAM=/usr/bin/bresca-welcome
MENU_ENTRY=/usr/share/applications/com.brescaos.welcome.desktop
AUTOSTART_ENTRY=/etc/xdg/autostart/com.brescaos.welcome-autostart.desktop

setup() {
    export QT_QPA_PLATFORM=offscreen XDG_STATE_HOME=$BATS_TEST_TMPDIR
}

@test "the program and its .desktop files are there" {
    local path
    for path in "$PROGRAM" "$MENU_ENTRY" "$AUTOSTART_ENTRY"; do
        [[ -f $path ]] || {
            echo "missing: $path"
            return 1
        }
    done
    grep -qx 'Exec=bresca-welcome --login' "$AUTOSTART_ENTRY"
}

@test "the program and its .desktop files are translated" {
    local lang path
    for lang in es ca; do
        [[ -f /usr/share/locale/$lang/LC_MESSAGES/bresca-welcome.mo ]] || {
            echo "missing the translations: $lang"
            return 1
        }
        for path in "$MENU_ENTRY" "$AUTOSTART_ENTRY"; do
            grep -q "^Name\[$lang\]=" "$path" || {
                echo "not translated: $path ($lang)"
                return 1
            }
        done
    done
}

@test "its version is the image's" {
    local version_id
    version_id=$(. /usr/lib/os-release && echo "$VERSION_ID")
    run "$PROGRAM" --version
    [[ $status -eq 0 && $output == "bresca-welcome $version_id" ]]
}

# Still running after a few seconds: its QML loaded, with the image's
# modules, and its window is up.
@test "at the first login it opens and records the revision" {
    run timeout 5 "$PROGRAM" --login
    [[ $status -eq 124 ]] || {
        echo "exited with $status instead of showing its window"
        return 1
    }
    grep -q '^ShownRevision=' "$XDG_STATE_HOME/bresca-welcomerc"
}

@test "with the revision already shown it doesn't open" {
    timeout 5 "$PROGRAM" --login || true
    run timeout 5 "$PROGRAM" --login
    [[ $status -eq 0 ]]
}
