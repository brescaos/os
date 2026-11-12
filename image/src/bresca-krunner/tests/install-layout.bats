#!/usr/bin/env bats
# The plugin and KRunner's defaults are where image/Containerfile installs
# them, the plugin translated.

PLUGIN=/usr/lib64/qt6/plugins/kf6/krunner/krunner_brescaos.so

@test "the plugin is there" {
    [[ -f $PLUGIN ]] || {
        echo "missing: $PLUGIN"
        return 1
    }
}

@test "KRunner's first favorite is the plugin" {
    grep -q "^plugins=krunner_brescaos," /etc/xdg/krunnerrc
}

@test "the plugin and its name are translated" {
    local lang
    for lang in es ca; do
        [[ -f /usr/share/locale/$lang/LC_MESSAGES/krunner_brescaos.mo ]] || {
            echo "missing the translations: $lang"
            return 1
        }
        # Its metadata is compiled in.
        grep -q -a "Description\[$lang\]" "$PLUGIN" || {
            echo "the plugin's metadata isn't translated: $lang"
            return 1
        }
    done
}
