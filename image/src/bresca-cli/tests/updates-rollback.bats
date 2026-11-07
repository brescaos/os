#!/usr/bin/env bats
# bresca updates rollback, through bresca-updates@rollback.

load helpers

@test "the previous version boots next; run again, the current one" {
    keep_rollback_version 0.0.9
    run bresca updates rollback
    [[ $status -eq 0 && $output == *"BrescaOS 0.0.9 boots next. Restart to use it"* ]]
    [[ $(jq -r .status.rollbackQueued "$BOOTC_STATE") == true ]]
    run bresca updates rollback
    [[ $status -eq 0 && $output == *"BrescaOS 0.1.0 (the current one) boots next again."* ]]
}

@test "no previous version: nothing to roll back to" {
    run bresca updates rollback
    [[ $status -eq 0 && $output == *"Nothing to roll back to"* ]]
}

@test "a staged version is kept unless --discard-staged, and nothing starts" {
    keep_rollback_version 0.0.9
    stage_version 0.2.0 downloaded
    run --separate-stderr bresca updates rollback
    [[ $status -eq 1 && $stderr == *"BrescaOS 0.2.0 is downloaded"*"--discard-staged"* ]]
    [[ ! -s $SYSTEMCTL_LOG && $(staged_field image.version) == 0.2.0 ]]
}

@test "--discard-staged rolls back and the staged version goes" {
    keep_rollback_version 0.0.9
    stage_version 0.2.0 installed
    run bresca updates rollback --discard-staged
    [[ $status -eq 0 && $output == *"BrescaOS 0.0.9 boots next."* ]]
    [[ $(jq -r .status.staged "$BOOTC_STATE") == null ]]
}

@test "--reboot restarts when the previous version boots next, not when undoing" {
    keep_rollback_version 0.0.9
    run bresca updates rollback --reboot
    grep -qx reboot "$SYSTEMCTL_LOG"
    : >"$SYSTEMCTL_LOG"
    run bresca updates rollback --reboot
    # Not "! grep": bash ignores a negated command for set -e, so it never fails.
    run grep -qx reboot "$SYSTEMCTL_LOG"
    [[ $status -ne 0 ]]
}

@test "a failed rollback fails with its error" {
    keep_rollback_version 0.0.9
    export BOOTC_FAILS=rollback
    run --separate-stderr bresca updates rollback
    [[ $status -eq 1 && $stderr == *"couldn't roll back"* ]]
}
