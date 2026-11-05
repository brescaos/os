#!/usr/bin/env bash
# Runs inside the image build (see Containerfile)
# the version-independent setup, cached across versions. Fedora's release
# packages swapped for generic ones and os-release without the version.
set -euo pipefail

die() {
    echo "build/system.sh: $*" >&2
    exit 1
}

# --- Fedora trademarks ----------------------------------------------------------
# Fedora's trademark guidelines: a system that combines Fedora with other
# software must not ship fedora-logos, fedora-release or fedora-release-notes
# (README, "Fedora trademarks"). Must use generic-release and generic-logos instead.
# However, some configuration file are missing 
# from these packages. Copy Fedora presets and dnf defaults
# under BrescaOS names before remove these packages. 
# Presets are read in lexical order and the first match wins, so 90-brescaos-default.preset
# comes before generic's 90-default.preset.
fedora_pkgs=$(rpm -qa --qf '%{NAME}\n' 'fedora-release*' fedora-logos | sort -u)
while IFS= read -r f; do
    [[ -f $f ]] || continue
    dir=${f%/*} name=${f##*/}
    case $f in
        */systemd/system-preset/[0-9][0-9]-*.preset | */systemd/user-preset/[0-9][0-9]-*.preset)
            copy="$dir/${name:0:3}brescaos-${name:3}" ;;
        /usr/share/dnf5/libdnf.conf.d/*fedora*)
            copy="$dir/${name/fedora/brescaos}" ;;
        *) continue ;;
    esac
    cp "$f" "$copy"
    echo "build/system.sh: $f copied to $copy"
done < <(xargs rpm -ql <<< "$fedora_pkgs")
for f in /usr/lib/systemd/system-preset/85-brescaos-display-manager.preset \
         /usr/lib/systemd/system-preset/90-brescaos-default.preset \
         /usr/lib/systemd/user-preset/90-brescaos-default-user.preset \
         /usr/share/dnf5/libdnf.conf.d/20-brescaos-defaults.conf; do
    [[ -f $f ]] || die "$f missing, check fedora-release-common's file list"
done
# MIT requires its notice to stay with those copies, and the package that has
# it goes below.
install -D -m 644 /usr/share/licenses/fedora-release-common/LICENSE \
    /usr/share/licenses/brescaos/fedora-release-common-LICENSE \
    || die "fedora-release-common's LICENSE missing, check its file list"

# The end of Fedora's support for this release, kept in os-release below.
support_end=$(. /usr/lib/os-release && echo "${SUPPORT_END:-}")

# Installed packages that need fedora-release-common and must be removed too.
fedora_only=$(rpm -qa --qf '%{NAME}\n' console-login-helper-messages-issuegen)

# One transaction, so nothing that needs system-release or system-logos is
# ever left without it. dnf's cache is a build cache mount (see Containerfile).
before=$(rpm -qa --qf '%{NAME}\n' | sort)
# shellcheck disable=SC2086 # one package name per word
# "do" quoted: dnf's command, not the shell keyword.
dnf -y --refresh --setopt=install_weak_deps=False --setopt=keepcache=True "do" \
    --action=remove $fedora_pkgs $fedora_only \
    --action=install generic-release generic-logos
removed=$(comm -23 <(echo "$before") <(rpm -qa --qf '%{NAME}\n' | sort))
# shellcheck disable=SC2086 # one package name per word
unexpected=$(comm -23 <(echo "$removed") <(printf '%s\n' $fedora_pkgs $fedora_only | sort))
# shellcheck disable=SC2086 # one line, whatever the number of packages
[[ -z $unexpected ]] || die "replacing Fedora's release packages also removed:" $unexpected
left=$(rpm -qa 'fedora-release*' fedora-logos)
# shellcheck disable=SC2086 # one line, whatever the number of packages
[[ -z $left ]] || die "Fedora's release packages are still installed:" $left

# --- Identity (version-independent part) -----------------------------------------
# Written whole: generic-release's os-release names a "Generic" system. ID
# stays "fedora" because tools tell the distribution apart by it (it isn't
# shown as the system's name). build/identity.sh appends the version lines.
# dnf resolves $releasever from generic-release's system-release(N), not from
# VERSION_ID, so VERSION_ID can hold BrescaOS's.
{
    printf 'NAME="BrescaOS"\n'
    printf 'ID=fedora\n'
    printf 'PRETTY_NAME="BrescaOS"\n'
    printf 'DEFAULT_HOSTNAME="bresca"\n'
    printf 'IMAGE_ID=brescaos\n'
    [[ -z $support_end ]] || printf 'SUPPORT_END=%s\n' "$support_end"
    printf 'HOME_URL="https://www.brescaos.com"\n'
} > /usr/lib/os-release
