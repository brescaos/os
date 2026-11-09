#!/usr/bin/env bats
# The applet's files are where image/Containerfile installs them, with what it
# runs.

@test "the applet's package and its notifications are there" {
    local path
    for path in /usr/share/plasma/plasmoids/com.brescaos.updates/metadata.json \
        /usr/share/plasma/plasmoids/com.brescaos.updates/contents/ui/main.qml \
        /usr/share/knotifications6/brescaos-updates.notifyrc; do
        [[ -f $path ]] || {
            echo "missing: $path"
            return 1
        }
    done
}

@test "the applet, its notifications and its metadata are translated" {
    local lang
    for lang in es ca; do
        [[ -f /usr/share/locale/$lang/LC_MESSAGES/plasma_applet_com.brescaos.updates.mo ]] || {
            echo "missing the translations: $lang"
            return 1
        }
        grep -q "^Name\[$lang\]=" /usr/share/knotifications6/brescaos-updates.notifyrc || {
            echo "the notifyrc isn't translated: $lang"
            return 1
        }
        jq -e --arg key "Name[$lang]" '.KPlugin[$key]' \
            /usr/share/plasma/plasmoids/com.brescaos.updates/metadata.json >/dev/null || {
            echo "metadata.json isn't translated: $lang"
            return 1
        }
    done
}

@test "inotifywait, which waits for check.json to change, is installed" {
    command -v inotifywait
}
