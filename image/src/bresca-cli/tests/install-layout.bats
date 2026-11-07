#!/usr/bin/env bats
# The CLI's files are where image/Containerfile installs them.

load helpers

@test "commands, operations and the dispatcher are executable" {
    local path
    for path in /usr/bin/bresca /usr/libexec/bresca/version \
        /usr/libexec/bresca/updates/{status,check,download,install,rollback,registry} \
        /usr/libexec/bresca/services/updates/{check,download,download-install,install,rollback}; do
        [[ -x $path ]] || {
            echo "not executable: $path"
            return 1
        }
    done
}

@test "libraries, the unit template and the completion are there" {
    local path
    for path in /usr/libexec/bresca/lib/{messages,registry-tls,download-progress}.sh \
        /usr/lib/systemd/system/bresca-updates@.service \
        /usr/share/bash-completion/completions/bresca; do
        [[ -f $path ]] || {
            echo "missing: $path"
            return 1
        }
    done
}

@test "jq is installed" {
    command -v jq
}
