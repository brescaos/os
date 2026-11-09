#!/usr/bin/env bash
# Compiles the translations of image/src/ (docs/translations.md) into
# <out dir>, laid out as in the image:
# - every <component>/po/<lang>.po as
#   usr/share/locale/<lang>/LC_MESSAGES/<domain>.mo, <domain> being the name of
#   the component's .pot;
# - the updates applet's notifyrc and metadata.json, with Name[<lang>]=... and
#   "Name[<lang>]" from its .po files.
# Runs in image/Containerfile's translations stage and in the development VM's
# hot reload, with gettext and jq.
# Usage: translations.sh <src dir> <out dir>
set -euo pipefail

src_dir=$1
out_dir=$2

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
metadata_dir=$out_dir/usr/share/plasma/plasmoids/com.brescaos.updates
mkdir -p "$(dirname "$notifyrc")" "$metadata_dir"
cp "$applet_dir/brescaos-updates.notifyrc.in" "$notifyrc"
metadata=$(<"$applet_dir/metadata.json.in")

for po in "$applet_dir"/po/*.po; do
    lang=$(basename "$po" .po)
    # Adds this language's Name[<lang>]= and Comment[<lang>]= lines.
    msgfmt --desktop -l "$lang" --template="$notifyrc" -o "$notifyrc.new" "$po"
    mv "$notifyrc.new" "$notifyrc"
    # {msgid: msgstr} of every translated message.
    translations=$(msgexec -i "$po" sh -c \
        'jq -n --arg msgid "$MSGEXEC_MSGID" --rawfile msgstr /dev/stdin "{(\$msgid): \$msgstr}"' \
        | jq -s 'add | with_entries(select(.key != "" and .value != ""))')
    metadata=$(jq --arg lang "$lang" --argjson translations "$translations" '
        .KPlugin |= . + ({("Name[\($lang)]"): $translations[.Name],
                          ("Description[\($lang)]"): $translations[.Description]}
                         | with_entries(select(.value != null)))' <<<"$metadata")
done
printf '%s\n' "$metadata" >"$metadata_dir/metadata.json"
