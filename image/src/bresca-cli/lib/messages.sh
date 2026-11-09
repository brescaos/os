# shellcheck shell=bash
# Sourced first by every bresca script: its messages are $"..." strings, which
# bash translates from the bresca domain (po/, compiled to
# /usr/share/locale/<lang>/LC_MESSAGES/bresca.mo) when it reads them, so this
# must come before any. Messages and errors in the user's language; warnings and
# errors on stderr, with a symbol, and in color on a terminal.

# shellcheck disable=SC2034 # bash reads it to translate $"..."
TEXTDOMAIN=bresca
# A translation is taken as text: it can't run commands or expand variables.
shopt -s noexpand_translation

# The line <format> on stdout: a $"..." message with printf's %s for
# <arguments>, so no message carries its \n.
print_message() {
    local format=$1
    shift
    printf "$format\n" "$@"
}

# ⚠ and ✗ need a UTF-8 locale and a font that has them; the Linux text
# console (TERM=linux) doesn't.
can_show_unicode_symbols() {
    [[ $(locale charmap 2>/dev/null) == UTF-8 && ${TERM:-} != linux ]]
}

# "⚠ warning: <message>", in yellow; "(!)" without ⚠. Takes print_message's
# arguments.
print_warning() {
    local symbol='(!)' yellow='' reset=''
    if can_show_unicode_symbols; then
        symbol='⚠'
    fi
    if [[ -t 2 ]]; then
        yellow=$'\e[1;33m' reset=$'\e[0m'
    fi
    printf '%s%s %s%s ' "$yellow" "$symbol" $"warning:" "$reset" >&2
    print_message "$@" >&2
}

# "✗ error: <message>", in red; "(x)" without ✗. Takes print_message's
# arguments.
print_error() {
    local symbol='(x)' red='' reset=''
    if can_show_unicode_symbols; then
        symbol='✗'
    fi
    if [[ -t 2 ]]; then
        red=$'\e[1;31m' reset=$'\e[0m'
    fi
    printf '%s%s %s%s ' "$red" "$symbol" $"error:" "$reset" >&2
    print_message "$@" >&2
}
