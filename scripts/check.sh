#!/usr/bin/env bash
# Checks (make check): shellcheck and bash syntax on every shell script.
set -euo pipefail
. "$(dirname "$0")/lib/common.sh"

command -v shellcheck >/dev/null 2>&1 \
    || fail "shellcheck not found: install it to run make check"

cd "$REPO_ROOT"

# Every shell script in the repository: *.sh, or a bash/sh shebang.
files=$(
    find . -path ./.git -prune -o -type f -print \
        | sort \
        | while IFS= read -r f; do
            case $f in
                *.sh) printf '%s\n' "${f#./}" ;;
                # if, not &&: a non-script as the last file must not fail the pipeline
                *) if head -n 1 "$f" 2>/dev/null | grep -qE '^#!.*[/ ](ba)?sh$'; then
                       printf '%s\n' "${f#./}"
                   fi ;;
            esac
        done
)
[ -n "$files" ] || fail "no shell scripts found"

status=0
for f in $files; do
    bash -n "$f" || status=1
done
# shellcheck disable=SC2086 # word splitting over the file list is intended
shellcheck -x -S warning -s bash $files || status=1

[ "$status" -eq 0 ] || fail "checks failed"
ok "shellcheck and syntax clean"
