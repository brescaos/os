#!/usr/bin/env bats
# The Distrobox tray applet and its QML module are where image/Containerfile
# installs them, translated. Its QML needs Plasma: the development VM tries it.

APPLET_DIR=/usr/share/plasma/plasmoids/com.brescaos.distrobox
QML_MODULE_DIR=/usr/lib64/qt6/qml/com/brescaos/distrobox

@test "the applet and its QML module are there" {
    local path
    for path in "$APPLET_DIR/metadata.json" "$APPLET_DIR/contents/ui/main.qml" \
        "$APPLET_DIR/contents/icons/distrobox-symbolic.svg" "$QML_MODULE_DIR/qmldir" \
        /usr/share/icons/hicolor/scalable/apps/com.brescaos.distrobox.svg; do
        [[ -f $path ]] || {
            echo "missing: $path"
            return 1
        }
    done
}

@test "the applet is in the system tray, for every user" {
    jq -e '."X-Plasma-NotificationArea" == "true" and .KPlugin.EnabledByDefault == true' "$APPLET_DIR/metadata.json" >/dev/null
}

@test "the translations and the metadata are translated" {
    local lang
    for lang in es ca; do
        [[ -f /usr/share/locale/$lang/LC_MESSAGES/plasma_applet_com.brescaos.distrobox.mo ]] || {
            echo "missing the translations: $lang"
            return 1
        }
        jq -e --arg key "Description[$lang]" '.KPlugin[$key]' "$APPLET_DIR/metadata.json" >/dev/null || {
            echo "the metadata isn't translated: $lang"
            return 1
        }
    done
}

@test "the QML module's plugin loads with what the image has" {
    local plugin
    plugin=$(echo "$QML_MODULE_DIR"/*.so)
    [[ -f $plugin ]]
    ! ldd "$plugin" | grep 'not found'
}
