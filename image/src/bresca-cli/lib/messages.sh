# shellcheck shell=bash
# Sourced by the bresca commands: warnings and errors on stderr, with a
# symbol, and in color on a terminal.

# ⚠ and ✗ need a UTF-8 locale and a font that has them; the Linux text
# console (TERM=linux) doesn't.
can_show_unicode_symbols() {
    [[ $(locale charmap 2>/dev/null) == UTF-8 && ${TERM:-} != linux ]]
}

# "⚠ warning: <message>", in yellow; "(!)" without ⚠.
print_warning() {
    local message=$1 symbol='(!)' yellow='' reset=''
    if can_show_unicode_symbols; then
        symbol='⚠'
    fi
    if [[ -t 2 ]]; then
        yellow=$'\e[1;33m' reset=$'\e[0m'
    fi
    printf '%s%s warning:%s %s\n' "$yellow" "$symbol" "$reset" "$message" >&2
}

# "✗ error: <message>", in red; "(x)" without ✗.
print_error() {
    local message=$1 symbol='(x)' red='' reset=''
    if can_show_unicode_symbols; then
        symbol='✗'
    fi
    if [[ -t 2 ]]; then
        red=$'\e[1;31m' reset=$'\e[0m'
    fi
    printf '%s%s error:%s %s\n' "$red" "$symbol" "$reset" "$message" >&2
}
