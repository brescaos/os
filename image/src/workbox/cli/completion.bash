# shellcheck shell=bash
# Bash completion for workbox. Keep the commands and options in sync with
# their help texts (main.cpp). bash-completion loads this file the first time
# "workbox" is completed.

_comp_cmd_workbox() {
    # shellcheck disable=SC2034 # prev and comp_args are set for _comp_initialize
    local cur prev words cword comp_args
    _comp_initialize -- "$@" || return

    if ((cword == 1)); then
        _comp_compgen -- -W 'create list info start stop enter rm --help'
    elif [[ ${words[1]} == create && $cur == -* ]]; then
        _comp_compgen -- -W '--name --color --sudo --help'
    elif [[ ${words[1]} == list && $cur == -* ]]; then
        _comp_compgen -- -W '--quiet --help'
    elif [[ ${words[1]} == rm && $cur == -* ]]; then
        _comp_compgen -- -W '--yes --help'
    elif [[ ${words[1]} == enter && $cur == -* ]]; then
        _comp_compgen -- -W '--root --help'
    elif [[ ${words[1]} == @(info|start|stop) && $cur == -* ]]; then
        _comp_compgen -- -W '--help'
    elif [[ ${words[1]} == @(info|start|stop|enter|rm) ]]; then
        # The ids of the workboxes there are; nothing when workbox fails.
        _comp_compgen_split -- "$(workbox list --quiet 2>/dev/null)"
    fi
} \
    && complete -F _comp_cmd_workbox workbox
