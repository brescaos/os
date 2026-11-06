#!/usr/bin/env bash
# Runs inside the image build (see Containerfile).
# Arguments: <brescaos-version> <fedora-version>
set -euo pipefail

version=$1
fedora_version=$2

# The version lines of os-release (build/system.sh writes the rest).
{
    printf 'VERSION="%s"\n' "$version"
    printf 'VERSION_ID=%s\n' "$version"
    printf 'IMAGE_VERSION=%s\n' "$version"
} >>/usr/lib/os-release
# Read through /etc/system-release and /etc/redhat-release.
printf 'BrescaOS release %s (based on Fedora %s)\n' "$version" "$fedora_version" >/usr/lib/fedora-release
printf 'cpe:/o:brescaos:brescaos:%s\n' "$version" >/usr/lib/system-release-cpe
