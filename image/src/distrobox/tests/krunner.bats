#!/usr/bin/env bats
# The KRunner plugin and its notifications' application are where
# image/Containerfile installs them, translated. What it finds has its own
# tests (make check-distrobox); what it does only runs in the development VM.

PLUGIN=/usr/lib64/qt6/plugins/kf6/krunner/krunner_distrobox.so
DESKTOP_FILE=/usr/share/applications/com.brescaos.distrobox.desktop

@test "the plugin and its notifications' application are there" {
    local path
    for path in "$PLUGIN" "$DESKTOP_FILE"; do
        [[ -f $path ]] || {
            echo "missing: $path"
            return 1
        }
    done
}

@test "the plugin loads with what the image has" {
    ! ldd "$PLUGIN" | grep 'not found'
}

@test "the application is hidden from the menu" {
    grep -q "^NoDisplay=true$" "$DESKTOP_FILE"
}

@test "the plugin and its notifications' application are translated" {
    local lang
    for lang in es ca; do
        grep -q -a "Description\[$lang\]" "$PLUGIN" || {
            echo "the plugin's metadata isn't translated: $lang"
            return 1
        }
        grep -q "^Comment\[$lang\]=" "$DESKTOP_FILE" || {
            echo "the .desktop file isn't translated: $lang"
            return 1
        }
    done
}
