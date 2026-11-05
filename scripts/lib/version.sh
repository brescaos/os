# BrescaOS versions (X.Y.Z or X.Y.Z-<alpha|beta|rc>.N): validation.
# shellcheck shell=bash
# shellcheck disable=SC2034 # used by the scripts that source lib/common.sh

# X.Y.Z or X.Y.Z-<alpha|beta|rc>.N, without leading zeros; N starts at 1.
VERSION_RE='^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)(-(alpha|beta|rc)\.([1-9][0-9]*))?$'

require_version() {
    [ -n "$VERSION" ] \
        || fail "set VERSION (e.g. VERSION=1.2.5, VERSION=1.2.5-rc.1, VERSION=2.1.0-alpha.1)"
    [[ $VERSION =~ $VERSION_RE ]] \
        || fail "invalid VERSION '$VERSION': use X.Y.Z or X.Y.Z-<alpha|beta|rc>.N (e.g. 1.2.5, 1.2.5-rc.1, 2.1.0-alpha.1)"
}
