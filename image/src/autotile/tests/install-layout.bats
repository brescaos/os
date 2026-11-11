#!/usr/bin/env bats
# Autotiling's files are where image/Containerfile installs them, translated.

SCRIPT_DIR=/usr/share/kwin/scripts/com.brescaos.autotile
APPLET_DIR=/usr/share/plasma/plasmoids/com.brescaos.autotile
MODULE=/usr/lib64/qt6/plugins/plasma/kcms/systemsettings/kcm_brescaos_autotile.so
DESKTOP_FILE=/usr/share/applications/kcm_brescaos_autotile.desktop
QML_MODULE_DIR=/usr/lib64/qt6/qml/com/brescaos/autotile

@test "the script, the applet, the KCM and the QML module are there" {
    local path
    for path in "$SCRIPT_DIR/metadata.json" "$SCRIPT_DIR/contents/code/main.js" \
        "$APPLET_DIR/metadata.json" "$APPLET_DIR/contents/ui/main.qml" \
        "$MODULE" "$DESKTOP_FILE" "$QML_MODULE_DIR/qmldir"; do
        [[ -f $path ]] || {
            echo "missing: $path"
            return 1
        }
    done
}

@test "the script has its layouts, in one file, and is off by default" {
    grep -q '^function createBinaryTree()' "$SCRIPT_DIR/contents/code/main.js"
    grep -q '^const LAYOUTS = ' "$SCRIPT_DIR/contents/code/main.js"
    jq -e '.KPlugin.EnabledByDefault == false' "$SCRIPT_DIR/metadata.json" >/dev/null
}

@test "the translations, the metadata and the .desktop are translated" {
    local lang metadata
    for lang in es ca; do
        [[ -f /usr/share/locale/$lang/LC_MESSAGES/kcm_brescaos_autotile.mo ]] || {
            echo "missing the translations: $lang"
            return 1
        }
        for metadata in "$SCRIPT_DIR/metadata.json" "$APPLET_DIR/metadata.json"; do
            jq -e --arg key "Name[$lang]" '.KPlugin[$key]' "$metadata" >/dev/null || {
                echo "$metadata isn't translated: $lang"
                return 1
            }
        done
        # The KCM's metadata is compiled in.
        grep -q -a "Name\[$lang\]" "$MODULE" || {
            echo "the module's metadata isn't translated: $lang"
            return 1
        }
        grep -q "^Name\[$lang\]=" "$DESKTOP_FILE" || {
            echo "the .desktop isn't translated: $lang"
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
