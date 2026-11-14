#!/usr/bin/env bats
# Hexa's files are where its Containerfile installs them, with what Plasma
# reads from them.

THEMES="com.brescaos.hexa.light.desktop com.brescaos.hexa.dark.desktop"

@test "both color schemes are there, each with its own name" {
    grep -qx 'ColorScheme=HexaLight' /usr/share/color-schemes/HexaLight.colors
    grep -qx 'ColorScheme=HexaDark' /usr/share/color-schemes/HexaDark.colors
}

@test "each Global Theme has its metadata and defaults, with its color scheme" {
    local theme dir
    for theme in $THEMES; do
        dir=/usr/share/plasma/look-and-feel/$theme
        jq -e --arg id "$theme" '.KPlugin.Id == $id' "$dir/metadata.json" >/dev/null || {
            echo "metadata.json without its Id: $theme"
            return 1
        }
        grep -q '^ColorScheme=Hexa' "$dir/contents/defaults" || {
            echo "defaults without a Hexa color scheme: $theme"
            return 1
        }
    done
}

@test "the Global Themes' metadata is translated" {
    local theme lang
    for theme in $THEMES; do
        for lang in es ca; do
            jq -e --arg key "Description[$lang]" '.KPlugin[$key]' \
                "/usr/share/plasma/look-and-feel/$theme/metadata.json" >/dev/null || {
                echo "metadata.json isn't translated: $theme, $lang"
                return 1
            }
        done
    done
}
