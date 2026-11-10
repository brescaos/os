#!/usr/bin/env bash
# Compiles the translations of image/src/ (docs/translations.md):
# - into <out dir>, laid out as in the image:
#   - every <component>/po/<lang>.po as
#     usr/share/locale/<lang>/LC_MESSAGES/<domain>.mo, <domain> being the name
#     of the component's .pot;
#   - the updates applet's notifyrc and metadata.json, with Name[<lang>]=...
#     and "Name[<lang>]" from its .po files;
# - into <metadata dir>, what a build compiles in: the updates KCM's
#   updates-kcm/kcm_brescaos_updates.json, with "Name[<lang>]" and the like.
# Runs in image/Containerfile's translations stage and in the development VM's
# hot reload, with gettext and jq.
# Usage: compile_translations.sh <src dir> <out dir> <metadata dir>
set -euo pipefail

src_dir=$1
out_dir=$2
metadata_dir=$3

# Writes the plugin metadata $1 to $3 with the translations of its Name,
# Description and X-KDE-Keywords from the .po files in the folder $2.
translate_metadata() {
    local template=$1 po_dir=$2 output=$3 metadata po lang translations
    metadata=$(<"$template")
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
            + translated("X-KDE-Keywords")' <<<"$metadata")
    done
    mkdir -p "$(dirname "$output")"
    printf '%s\n' "$metadata" >"$output"
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
notifyrc=$out_dir/usr/share/knotifications6/brescaos-updates.notifyrc
mkdir -p "$(dirname "$notifyrc")"
cp "$applet_dir/brescaos-updates.notifyrc.in" "$notifyrc"
for po in "$applet_dir"/po/*.po; do
    # Adds this language's Name[<lang>]= and Comment[<lang>]= lines.
    msgfmt --desktop -l "$(basename "$po" .po)" --template="$notifyrc" -o "$notifyrc.new" "$po"
    mv "$notifyrc.new" "$notifyrc"
done
translate_metadata "$applet_dir/metadata.json.in" "$applet_dir/po" \
    "$out_dir/usr/share/plasma/plasmoids/com.brescaos.updates/metadata.json"

translate_metadata "$src_dir/updates-kcm/kcm_brescaos_updates.json.in" "$src_dir/updates-kcm/po" \
    "$metadata_dir/updates-kcm/kcm_brescaos_updates.json"
