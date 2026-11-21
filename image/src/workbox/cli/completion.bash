# shellcheck shell=bash
# Bash completion for workbox. Keep the commands and options in sync with
# their help texts (main.cpp). bash-completion loads this file the first time
# "workbox" is completed.

_comp_cmd_workbox() {
    # shellcheck disable=SC2034 # prev and comp_args are set for _comp_initialize
    local cur prev words cword comp_args
    _comp_initialize -- "$@" || return

    if ((cword == 1)); then
        _comp_compgen -- -W 'create list start stop rm --help'
    elif [[ ${words[1]} == create && $cur == -* ]]; then
        _comp_compgen -- -W '--name --color --help'
    elif [[ ${words[1]} == list && $cur == -* ]]; then
        _comp_compgen -- -W '--quiet --help'
    elif [[ ${words[1]} == rm && $cur == -* ]]; then
        _comp_compgen -- -W '--yes --help'
    elif [[ ${words[1]} == @(start|stop) && $cur == -* ]]; then
        _comp_compgen -- -W '--help'
    elif [[ ${words[1]} == @(start|stop|rm) ]]; then
        # The ids of the workboxes there are; nothing when workbox fails.
        _comp_compgen_split -- "$(workbox list --quiet 2>/dev/null)"
    fi
} \
    && complete -F _comp_cmd_workbox workbox
