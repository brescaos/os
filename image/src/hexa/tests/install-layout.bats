#!/usr/bin/env bats
# Hexa's files are where its Containerfile installs them, with what Plasma
# reads from them.

THEMES="com.brescaos.hexa.light.desktop com.brescaos.hexa.dark.desktop"

@test "both color schemes are there, each with its own name" {
    grep -qx 'ColorScheme=HexaLight' /usr/share/color-schemes/HexaLight.colors
    grep -qx 'ColorScheme=HexaDark' /usr/share/color-schemes/HexaDark.colors
}

@test "both color schemes soften separators and frames" {
    grep -qx 'frameContrast=0.08' /usr/share/color-schemes/HexaLight.colors
    grep -qx 'frameContrast=0.08' /usr/share/color-schemes/HexaDark.colors
}

@test "each icon theme is there, on Breeze's, with Hexa's icons, and its Global Theme uses it" {
    local variant parent icon
    for variant in light dark; do
        parent=breeze
        [[ $variant == dark ]] && parent=breeze-dark
        grep -qx "Inherits=$parent,hicolor" "/usr/share/icons/hexa-$variant/index.theme"
        [[ -f /usr/share/icons/$parent/index.theme ]]
        for icon in klipper-symbolic data-warning dialog-warning emblem-warning data-error dialog-error emblem-error \
            audio-volume-{high,medium,low,muted}{,-symbolic} \
            microphone-sensitivity-{high,medium,low,muted}{,-symbolic} audio-input-microphone{,-symbolic} \
            video-display-brightness{,-symbolic} redshift-status-{on,day,off}{,-symbolic} lighttable{,-symbolic} \
            network-wired-activated{,-symbolic}; do
            [[ -f /usr/share/icons/hexa-$variant/status/$icon.svg ]] || {
                echo "missing the icon: hexa-$variant, $icon"
                return 1
            }
        done
        grep -qx "Theme=hexa-$variant" \
            "/usr/share/plasma/look-and-feel/com.brescaos.hexa.$variant.desktop/contents/defaults"
    done
}

@test "each Global Theme has its metadata, its defaults with its color scheme, and the layout" {
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
        [[ -f $dir/contents/layouts/org.kde.plasma.desktop-layout.js ]] || {
            echo "missing the desktop layout: $theme"
            return 1
        }
    done
}

@test "the top bar's containment is there, a panel containment, translated" {
    local dir=/usr/share/plasma/plasmoids/com.brescaos.hexa.topbar
    [[ -f $dir/contents/ui/main.qml && -f $dir/contents/config/main.xml ]]
    jq -e '.KPlugin.Id == "com.brescaos.hexa.topbar" and ."X-Plasma-ContainmentType" == "Panel"' \
        "$dir/metadata.json" >/dev/null
    jq -e '.KPlugin["Description[es]"] and .KPlugin["Description[ca]"]' "$dir/metadata.json" >/dev/null
}

@test "the clock is there, translated, and the top bar's template adds it" {
    local dir=/usr/share/plasma/plasmoids/com.brescaos.hexa.clock
    [[ -f $dir/contents/ui/main.qml && -f $dir/contents/ui/configGeneral.qml && -f $dir/contents/config/main.xml ]]
    jq -e '.KPlugin.Id == "com.brescaos.hexa.clock"' "$dir/metadata.json" >/dev/null
    jq -e '.KPlugin["Description[es]"] and .KPlugin["Description[ca]"]' "$dir/metadata.json" >/dev/null
    grep -q 'addWidget("com.brescaos.hexa.clock")' \
        /usr/share/plasma/layout-templates/com.brescaos.hexa.topbar/contents/layout.js
}

@test "the QML module the top bar imports is there" {
    local dir=/usr/lib64/qt6/qml/com/brescaos/hexa
    grep -q '^module com.brescaos.hexa$' "$dir/qmldir"
    grep -q 'WallpaperContrast' "$dir"/*.qmltypes
    grep -q '^import com.brescaos.hexa$' /usr/share/plasma/plasmoids/com.brescaos.hexa.topbar/contents/ui/main.qml
}

@test "the top bar's panel template is there, translated" {
    local dir=/usr/share/plasma/layout-templates/com.brescaos.hexa.topbar
    grep -q 'new Panel("com.brescaos.hexa.topbar")' "$dir/contents/layout.js"
    jq -e '.KPlugin.Id == "com.brescaos.hexa.topbar" and (."X-Plasma-ContainmentCategories" | index("panel"))' \
        "$dir/metadata.json" >/dev/null
    jq -e '.KPlugin["Name[es]"] and .KPlugin["Description[ca]"]' "$dir/metadata.json" >/dev/null
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
