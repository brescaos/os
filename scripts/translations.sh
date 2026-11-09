#!/usr/bin/env bash
# The translations of image/src/ (docs/translations.md): every component with
# a po/ folder has its strings in po/<domain>.pot and one po/<lang>.po for each
# of TRANSLATION_LANGUAGES (settings/settings.sh).
#   translations.sh pot    extracts the strings into the .pot and merges them
#                          into every .po, creating the missing ones (make pot)
#   translations.sh check  fails when a .pot is stale, a .po is missing or out
#                          of sync with it, or something is untranslated or
#                          fuzzy (make check)
set -euo pipefail
. "$(dirname "$0")/lib/common.sh"

require_cmd xgettext "Install gettext to work with translations."
require_cmd jq "Install jq to work with translations."

# --add-location=file: no line numbers, so a change that moves strings without
# changing them doesn't make the .pot stale.
XGETTEXT_OPTIONS='--from-code=UTF-8 --add-location=file --sort-by-file --package-name=BrescaOS'

# Writes the strings of the component in the current folder to the .pot $1.
extract_strings() {
    local pot=$1
    # shellcheck disable=SC2086 # XGETTEXT_OPTIONS is a list of options
    case ${PWD##*/} in
        bresca-cli)
            # xgettext warns about every $"...": bash used to expand what a
            # translation has, which lib/messages.sh turns off
            # (noexpand_translation).
            find bresca commands lib -type f | sort \
                | xargs xgettext $XGETTEXT_OPTIONS -L Shell -o "$pot" 2>&1 \
                | { grep -v 'the syntax \$"\.\.\." is deprecated' || true; } >&2
            ;;
        updates-applet)
            find package -name '*.qml' | sort \
                | xargs xgettext $XGETTEXT_OPTIONS -L JavaScript --keyword= \
                    --keyword=i18n:1 --keyword=i18nc:1c,2 --keyword=i18np:1,2 --keyword=i18ncp:1c,2,3 \
                    -o "$pot"
            xgettext $XGETTEXT_OPTIONS -L Desktop -j -o "$pot" brescaos-updates.notifyrc.in
            # xgettext doesn't read JSON: the strings of metadata.json.in as a .po.
            {
                printf 'msgid ""\nmsgstr "Content-Type: text/plain; charset=UTF-8\\n"\n\n'
                jq -r '.KPlugin.Name, .KPlugin.Description
                       | "#: metadata.json.in\nmsgid \(tojson)\nmsgstr \"\"\n"' metadata.json.in
            } >"$pot.metadata.po"
            xgettext $XGETTEXT_OPTIONS -L PO -j -o "$pot" "$pot.metadata.po"
            rm -f "$pot.metadata.po"
            ;;
        *) fail "no rule to extract the strings of ${PWD##*/} (scripts/translations.sh)" ;;
    esac
}

# Whether the .pot files $1 and $2 have the same content, but for their date.
is_same_pot() {
    [ "$(grep -v '^"POT-Creation-Date:' "$1")" = "$(grep -v '^"POT-Creation-Date:' "$2")" ]
}

update_translations() {
    local component pot new_pot lang po
    for component in */po; do
        component=${component%/po}
        pot=$(echo "$component"/po/*.pot)
        new_pot=$work_dir/$component.pot
        (cd "$component" && extract_strings "$new_pot")
        if is_same_pot "$pot" "$new_pot"; then
            ok "$pot: up to date"
        else
            cp "$new_pot" "$pot"
            ok "$pot: updated"
        fi
        for lang in $TRANSLATION_LANGUAGES; do
            po=$component/po/$lang.po
            if [ -f "$po" ]; then
                msgmerge --quiet --update --backup=none "$po" "$pot"
            else
                msginit --no-translator --locale="$lang" --input="$pot" --output-file="$po"
                ok "$po: created"
            fi
        done
    done
}

check_translations() {
    local component pot new_pot lang po statistics
    local status=0
    for component in */po; do
        component=${component%/po}
        pot=$(echo "$component"/po/*.pot)
        new_pot=$work_dir/$component.pot
        (cd "$component" && extract_strings "$new_pot")
        if ! is_same_pot "$pot" "$new_pot"; then
            warn "$pot is stale: run make pot"
            status=1
        fi
        for lang in $TRANSLATION_LANGUAGES; do
            po=$component/po/$lang.po
            if [ ! -f "$po" ]; then
                warn "$po is missing: run make pot"
                status=1
                continue
            fi
            # Every string of the code is in the .po, and the .po has no
            # string the code no longer has.
            if ! msgcmp --use-untranslated --use-fuzzy "$po" "$new_pot" 2>/dev/null \
                || ! msgcmp --use-untranslated --use-fuzzy "$new_pot" "$po" 2>/dev/null \
                || [ -n "$(msgattrib --only-obsolete "$po")" ]; then
                warn "$po is out of sync with the code: run make pot"
                status=1
            fi
            if ! statistics=$(msgfmt --check --statistics -o /dev/null "$po" 2>&1); then
                warn "$po: $statistics"
                status=1
                continue
            fi
            case $statistics in
                *untranslated* | *fuzzy*)
                    warn "$po: $statistics"
                    status=1
                    ;;
            esac
        done
    done
    [ "$status" -eq 0 ] || fail "translations aren't complete (docs/translations.md)"
    ok "translations complete: $TRANSLATION_LANGUAGES"
}

work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT
cd "$REPO_ROOT/image/src"

case ${1:-} in
    pot) update_translations ;;
    check) check_translations ;;
    *) fail "usage: scripts/translations.sh pot|check" ;;
esac
