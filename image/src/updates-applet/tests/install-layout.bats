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

@test "inotifywait, which waits for check.json to change, is installed" {
    command -v inotifywait
}
