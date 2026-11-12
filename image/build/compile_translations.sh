#!/usr/bin/env bash
# Compiles the translations of image/src/ (docs/translations.md):
# - into <out dir>, laid out as in the image:
#   - every <component>/po/<lang>.po as
#     usr/share/locale/<lang>/LC_MESSAGES/<domain>.mo, <domain> being the name
#     of the component's .pot;
#   - the updates applet's notifyrc and metadata.json, with Name[<lang>]=...
#     and "Name[<lang>]" from its .po files;
#   - the welcome's .desktop files (menu and autostart), with Name[<lang>]=...;
#   - the autotiling KWin script's and tray applet's metadata.json;
#   - the Distrobox tray applet's metadata.json;
#   - the KRunner plugin's .desktop file (its shortcut);
# - into <metadata dir>, what a build compiles in: the KCMs'
#   updates-kcm/kcm_brescaos_updates.json and
#   autotile/kcm_brescaos_autotile.json, and the KRunner plugin's
#   bresca-krunner/krunner_brescaos.json, with "Name[<lang>]" and the like.
# Every metadata.json and KCM JSON also gets the version in its component's
# VERSION.txt.
# Runs in image/Containerfile's translations stage and in the development VM's
# hot reload, with gettext and jq.
# Usage: compile_translations.sh <src dir> <out dir> <metadata dir>
set -euo pipefail

src_dir=$1
out_dir=$2
metadata_dir=$3

# Writes the plugin metadata $1 to $3 with the translations of its Name,
# Description and search keywords (Keywords, X-KDE-Keywords) from the .po
# files in the folder $2, and the version in the component's VERSION.txt ($2/..).
translate_metadata() {
    local template=$1 po_dir=$2 output=$3 metadata po lang translations
    metadata=$(jq --arg version "$(<"$po_dir/../VERSION.txt")" '.KPlugin.Version = $version' "$template")
    for po in "$po_dir"/*.po; do
        lang=$(basename "$po" .po)
        # {msgid: msgstr} of every translated message.
        translations=$(msgexec -i "$po" sh -c \
            'jq -n --arg msgid "$MSGEXEC_MSGID" --rawfile msgstr /dev/stdin "{(\$msgid): \$msgstr}"' \
            | jq -s 'add | with_entries(select(.key != "" and .value != ""))')
        metadata=$(jq --arg lang "$lang" --argjson translations "$translations" '
            def translated(key): {("\(key)[\($lang)]"): $translations[.[key] // ""]}
                                 | with_entries(select(.value != null));
            (.KPlugin |= . + translated("Name") + translated("Description"))
            + translated("Keywords") + translated("X-KDE-Keywords")' <<<"$metadata")
    done
    mkdir -p "$(dirname "$output")"
    printf '%s\n' "$metadata" >"$output"
}

# Writes the .desktop-style file $1 to $3 with the translations of its Name,
# Comment and Keywords from the .po files in the folder $2.
translate_desktop_file() {
    local template=$1 po_dir=$2 output=$3 po
    mkdir -p "$(dirname "$output")"
    cp "$template" "$output"
    for po in "$po_dir"/*.po; do
        # Adds this language's Name[<lang>]= lines and the like.
        msgfmt --desktop -l "$(basename "$po" .po)" --template="$output" -o "$output.new" "$po"
        mv "$output.new" "$output"
    done
}

for pot in "$src_dir"/*/po/*.pot; do
    domain=$(basename "$pot" .pot)
    for po in "$(dirname "$pot")"/*.po; do
        messages_dir=$out_dir/usr/share/locale/$(basename "$po" .po)/LC_MESSAGES
        mkdir -p "$messages_dir"
        msgfmt --check -o "$messages_dir/$domain.mo" "$po"
    done
done

applet_dir=$src_dir/updates-applet
translate_desktop_file "$applet_dir/brescaos-updates.notifyrc.in" "$applet_dir/po" \
    "$out_dir/usr/share/knotifications6/brescaos-updates.notifyrc"
translate_metadata "$applet_dir/metadata.json.in" "$applet_dir/po" \
    "$out_dir/usr/share/plasma/plasmoids/com.brescaos.updates/metadata.json"

translate_metadata "$src_dir/updates-kcm/kcm_brescaos_updates.json.in" "$src_dir/updates-kcm/po" \
    "$metadata_dir/updates-kcm/kcm_brescaos_updates.json"

autotile_dir=$src_dir/autotile
translate_metadata "$autotile_dir/script/metadata.json.in" "$autotile_dir/po" \
    "$out_dir/usr/share/kwin/scripts/com.brescaos.autotile/metadata.json"
translate_metadata "$autotile_dir/applet/metadata.json.in" "$autotile_dir/po" \
    "$out_dir/usr/share/plasma/plasmoids/com.brescaos.autotile/metadata.json"
translate_metadata "$autotile_dir/kcm_brescaos_autotile.json.in" "$autotile_dir/po" \
    "$metadata_dir/autotile/kcm_brescaos_autotile.json"

welcome_dir=$src_dir/welcome
translate_desktop_file "$welcome_dir/com.brescaos.welcome.desktop.in" "$welcome_dir/po" \
    "$out_dir/usr/share/applications/com.brescaos.welcome.desktop"
translate_desktop_file "$welcome_dir/com.brescaos.welcome-autostart.desktop.in" "$welcome_dir/po" \
    "$out_dir/etc/xdg/autostart/com.brescaos.welcome-autostart.desktop"

distrobox_dir=$src_dir/distrobox
translate_metadata "$distrobox_dir/applet/metadata.json.in" "$distrobox_dir/po" \
    "$out_dir/usr/share/plasma/plasmoids/com.brescaos.distrobox/metadata.json"

translate_metadata "$src_dir/bresca-krunner/krunner_brescaos.json.in" "$src_dir/bresca-krunner/po" \
    "$metadata_dir/bresca-krunner/krunner_brescaos.json"
translate_desktop_file "$src_dir/bresca-krunner/com.brescaos.bresca-krunner.desktop.in" "$src_dir/bresca-krunner/po" \
    "$out_dir/usr/share/applications/com.brescaos.bresca-krunner.desktop"
