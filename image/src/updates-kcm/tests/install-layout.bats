#!/usr/bin/env bats
# The module's files are where image/Containerfile installs them, translated.

MODULE=/usr/lib64/qt6/plugins/plasma/kcms/systemsettings/kcm_brescaos_updates.so
DESKTOP_FILE=/usr/share/applications/kcm_brescaos_updates.desktop

@test "the module and its .desktop are there" {
    local path
    for path in "$MODULE" "$DESKTOP_FILE"; do
        [[ -f $path ]] || {
            echo "missing: $path"
            return 1
        }
    done
}

@test "the module, its name and its .desktop are translated" {
    local lang
    for lang in es ca; do
        [[ -f /usr/share/locale/$lang/LC_MESSAGES/kcm_brescaos_updates.mo ]] || {
            echo "missing the translations: $lang"
            return 1
        }
        # Its metadata is compiled in.
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
