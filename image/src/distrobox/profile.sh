# shellcheck shell=sh
# A distrobox's colors and prompt (README.md). Mounted into every distrobox as
# /etc/profile.d/zz-brescaos-distrobox.sh, so it comes after Distrobox's
# distrobox_profile.sh and the image's own profile.d, and its prompt wins.
# POSIX, since sh and zsh source /etc/profile.d too, but only an interactive
# bash entered with colors (bresca-distrobox-colors) changes anything.
if [ -n "${BASH_VERSION:-}" ] && [ -n "${PS1:-}" ] && [ -n "${BRESCAOS_DISTROBOX_COLOR:-}" ]; then
    # OSC 11 and 10: the terminal's background and text. OSC 111 and 110 put
    # back the profile's on exit (Konsole ignores them without the ";").
    printf '\033]11;%s\007\033]10;#fcfcfc\007' "$BRESCAOS_DISTROBOX_COLOR"
    brescaos_restore_colors='\033]111;\007\033]110;\007'
    # OSC 777 tells Konsole it's in this distrobox, and when it leaves: its
    # own detection looks only once, often before the container has started.
    # Konsole then gives the tab and its footer label its color for it.
    if [ -n "${KONSOLE_DBUS_SESSION:-}" ]; then
        printf '\033]777;container;push;%s;distrobox\007' "$CONTAINER_ID"
        brescaos_restore_colors="$brescaos_restore_colors\\033]777;container;pop;;\\007"
    fi
    # shellcheck disable=SC2064 # expanded now: the variable is unset below
    trap "printf '$brescaos_restore_colors'" EXIT

    # "📦 name [user@host dir]$ ", the name bold in the label's color.
    brescaos_hex=${BRESCAOS_DISTROBOX_LABEL_COLOR#\#}
    brescaos_green_blue=${brescaos_hex#??}
    brescaos_rgb=$(printf '%d;%d;%d' "0x${brescaos_hex%????}" "0x${brescaos_green_blue%??}" "0x${brescaos_hex#????}")
    PS1="\[\033[1;38;2;${brescaos_rgb}m\]📦 ${CONTAINER_ID}\[\033[0m\] [\u@\h \W]\\\$ "
    unset brescaos_restore_colors brescaos_hex brescaos_green_blue brescaos_rgb
fi
