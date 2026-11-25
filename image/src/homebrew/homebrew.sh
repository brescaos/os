# shellcheck shell=sh
# Homebrew, once the user installed it (/usr/bin/brew): what it installs goes
# in PATH after the system's, so it never takes the place of BrescaOS's own
# commands; its manuals, info pages and brew's bash completion too.

homebrew_prefix=/var/opt/$(id -u)/.linuxbrew
if [ -x "$homebrew_prefix/bin/brew" ]; then
    # A non-login shell reads this again, with the login's PATH.
    case ":$PATH:" in
        *":$homebrew_prefix/bin:"*) ;;
        *)
            PATH=$PATH:$homebrew_prefix/bin:$homebrew_prefix/sbin
            # Their empty entry is man's and info's own folders.
            MANPATH=${MANPATH-}:$homebrew_prefix/share/man
            INFOPATH=$homebrew_prefix/share/info:${INFOPATH-}
            export PATH MANPATH INFOPATH
            ;;
    esac
    if [ -n "${BASH_VERSION-}" ] && [ -r "$homebrew_prefix/etc/bash_completion.d/brew" ]; then
        . "$homebrew_prefix/etc/bash_completion.d/brew"
    fi
fi
unset homebrew_prefix
