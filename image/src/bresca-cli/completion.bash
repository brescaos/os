# shellcheck shell=bash
# Bash completion for bresca. Keep the commands and options in sync with their
# usage texts. bash-completion loads this file the first time "bresca" is
# completed.

_comp_cmd_bresca() {
    # shellcheck disable=SC2034 # prev and comp_args are set for _comp_initialize
    local cur prev words cword comp_args
    # -n =: keep "--format=json" as one word (= is a word break by default).
    _comp_initialize -n = -- "$@" || return

    if ((cword == 1)); then
        _comp_compgen -- -W 'version updates help --help'
    elif [[ ${words[1]} == version && $cur == -* ]]; then
        _comp_compgen -- -W '--short --format=json --format=text --help'
    elif [[ ${words[1]} == updates ]] && ((cword == 2)); then
        _comp_compgen -- -W 'status check download install registry help --help'
    elif [[ ${words[1]} == updates && ${words[2]} == @(status|check|registry) && $cur == -* ]]; then
        _comp_compgen -- -W '--format=json --format=text --help'
    elif [[ ${words[1]} == updates && ${words[2]} == download && $cur == -* ]]; then
        _comp_compgen -- -W '--no-tls --format=json --format=text --help'
    elif [[ ${words[1]} == updates && ${words[2]} == install && $cur == -* ]]; then
        _comp_compgen -- -W '--download --no-tls --reboot --format=json --format=text --help'
    fi
} \
    && complete -F _comp_cmd_bresca bresca
