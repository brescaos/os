#!/usr/bin/env bats
# The CLI's files are where image/Containerfile installs them.

load helpers

@test "commands, operations and the dispatcher are executable" {
    local path
    for path in /usr/bin/bresca /usr/libexec/bresca/version \
        /usr/libexec/bresca/updates/{status,check,download,install,rollback,registry} \
        /usr/libexec/bresca/services/updates/{check,download,download-install,install,rollback,status}; do
        [[ -x $path ]] || {
            echo "not executable: $path"
            return 1
        }
    done
}

@test "libraries, units, tmpfiles and the completion are there" {
    local path
    for path in /usr/libexec/bresca/lib/{messages,registry,download-progress}.sh \
        /usr/lib/systemd/system/bresca-updates@.service \
        /usr/lib/systemd/system/bresca-updates-check.timer \
        /usr/lib/tmpfiles.d/bresca-updates.conf \
        /usr/share/polkit-1/rules.d/50-brescaos-updates.rules \
        /usr/share/bash-completion/completions/bresca; do
        [[ -f $path ]] || {
            echo "missing: $path"
            return 1
        }
    done
}

@test "the translations are there" {
    local lang
    for lang in es ca; do
        [[ -f /usr/share/locale/$lang/LC_MESSAGES/bresca.mo ]] || {
            echo "missing: $lang"
            return 1
        }
    done
}

@test "the periodic check is enabled" {
    [[ -e /etc/systemd/system/timers.target.wants/bresca-updates-check.timer ]]
}

@test "jq is installed" {
    command -v jq
}
