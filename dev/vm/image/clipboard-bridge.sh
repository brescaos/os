#!/usr/bin/env bash
# Copies the Wayland clipboard to X11, so spice-vdagent
# sees what Plasma apps copy.
# KWin only hands the Wayland clipboard to X11 while an X11 window has
# the focus, and spice-vdagent has no window. Text only. In the VM's image as
# /usr/libexec/bresca-dev/clipboard-bridge, started at login by
# /etc/xdg/autostart/bresca-dev-clipboard.desktop.
#
#   clipboard-bridge         watch the Wayland clipboard
#   clipboard-bridge copy    (run by wl-paste) put stdin on the X11 clipboard
set -euo pipefail

case ${1:-} in
    "")
        exec wl-paste --type text --watch "$0" copy
        ;;
    copy)
        # The trailing "." keeps trailing newlines through $(...).
        new_text=$(
            cat
            echo .
        )
        new_text=${new_text%.}
        [[ -n $new_text ]] || exit 0
        old_text=$(
            xclip -o -selection clipboard 2>/dev/null || true
            echo .
        )
        old_text=${old_text%.}
        # KWin mirrors the X11 clipboard back to Wayland: the same text comes
        # back here, and stops.
        [[ $new_text == "$old_text" ]] || printf '%s' "$new_text" | xclip -selection clipboard
        ;;
    *)
        echo "usage: $0 [copy]" >&2
        exit 1
        ;;
esac
