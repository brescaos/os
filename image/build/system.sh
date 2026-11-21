#!/usr/bin/env bash
# Runs inside the image build (see Containerfile).
# Applies the version-independent setup, cached across versions.
# Arguments: <fedora-version>

set -euo pipefail

die() {
    echo "build/system.sh: $*" >&2
    exit 1
}

fedora_version=$1

# --- Kernel arguments --------------------------------------------------------------
# quiet: without it, the kernel's messages on the console are too noisy.
mkdir -p /usr/lib/bootc/kargs.d
printf 'kargs = ["quiet"]\n' >/usr/lib/bootc/kargs.d/10-brescaos-quiet.toml

# --- Services ----------------------------------------------------------------------
# Secure by default: nothing listening on the network, nothing a desktop doesn't
# need (why each unit: docs/security.md).
# Mask bootc-fetch-apply-updates.timer in order to avoid automatic updates.
# The disabled units also go into a preset that comes
# before Fedora's (kept above as 90-brescaos-default.preset, which enables
# them): systemd applies presets on a system's first boot, and the first preset
# file that names a unit wins.
disabled_units="sshd.service sssd.service mdmonitor.service raid-check.timer nfs-client.target rpm-ostree-countme.timer"
systemctl mask bootc-fetch-apply-updates.timer
# shellcheck disable=SC2086 # one unit per word
systemctl disable $disabled_units
printf 'disable %s\n' $disabled_units >/usr/lib/systemd/system-preset/10-brescaos-disable.preset

# LLMNR: systemd-resolved answers Windows-style name queries on port 5355 of
# every network, which anyone on it can spoof.
mkdir -p /usr/lib/systemd/resolved.conf.d
printf '[Resolve]\nLLMNR=no\n' >/usr/lib/systemd/resolved.conf.d/50-brescaos.conf

# --- Firewall ------------------------------------------------------------------------
# default (shown as "BrescaOS default" among Fedora's zones), BrescaOS's own
# zone and firewalld's default one. A NetworkManager connection without a
# zone, as every new one is, uses it. Every incoming connection is dropped.
# Only DHCPv6 replies get in, or IPv6 couldn't be set up. Users' changes
# (Settings > Firewall, firewall-cmd --permanent) go to /etc/firewalld/zones/,
# over this one.
cat >/usr/lib/firewalld/zones/default.xml <<'ZONE'
<?xml version="1.0" encoding="utf-8"?>
<zone target="DROP">
  <short>BrescaOS default</short>
  <description>Every incoming connection is dropped.</description>
  <service name="dhcpv6-client"/>
</zone>
ZONE
firewall-offline-cmd --set-default-zone=default >/dev/null
[[ $(firewall-offline-cmd --get-default-zone) == default ]] \
    || die "firewalld's default zone isn't default, check /etc/firewalld/firewalld.conf"

# Remove Fedora's other zones. Kept: block, drop, trusted, and nm-shared (Wi-Fi hotspot)
# because firewalld and NetworkManager require them.
# The build fails on any other zone, e.g. a new one from Fedora.
rm -f /usr/lib/firewalld/zones/{FedoraServer,FedoraWorkstation,dmz,external,home,internal,public,work}.xml
zones=$(cd /usr/lib/firewalld/zones && ls)
[[ $zones == $'block.xml\ndefault.xml\ndrop.xml\nnm-shared.xml\ntrusted.xml' ]] \
    || die "unexpected firewalld zones in /usr/lib/firewalld/zones:"$'\n'"$zones"
# Fedora's gateway-* policies, for a system that routes other networks' traffic,
# go with the zones they name (dmz, internal, external…). Kept: allow-host-ipv6,
# which names none. The build fails on any other policy.
rm -f /usr/lib/firewalld/policies/gateway-*.xml
policies=$(cd /usr/lib/firewalld/policies && ls)
[[ $policies == allow-host-ipv6.xml ]] \
    || die "unexpected firewalld policies in /usr/lib/firewalld/policies:"$'\n'"$policies"

# Check firewalld's configuration here: with a broken one, firewalld exits
# without failing and leaves the firewall off.
firewall-offline-cmd --check-config \
    || die "firewalld's configuration doesn't load (firewall-offline-cmd --check-config)"
systemctl enable firewalld.service

# --- workbox ---------------------------------------------------------------------------
# Set default image of workbox.
mkdir -p /usr/share/defaults/workbox
printf '[General]\nimage=registry.fedoraproject.org/fedora-toolbox:%s\n' "$fedora_version" \
    >/usr/share/defaults/workbox/workbox.conf
