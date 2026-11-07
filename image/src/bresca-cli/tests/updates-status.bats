#!/usr/bin/env bats
# bresca updates status: the deployments and which boots next.

load helpers

@test "only the booted version: it boots next" {
    run bresca updates status --format=json
    [[ $status -eq 0 ]]
    [[ $(jq -r .current.version <<<"$output") == 0.1.0 ]]
    [[ $(jq -r .staged <<<"$output") == null ]]
    [[ $(jq -r .boots_next <<<"$output") == current ]]
}

@test "an installed staged version boots next" {
    stage_version 0.2.0 installed
    run bresca updates status --format=json
    [[ $(jq -r .staged.version <<<"$output") == 0.2.0 ]]
    [[ $(jq -r .boots_next <<<"$output") == staged ]]
}

@test "a downloaded staged version doesn't boot: the current one does" {
    stage_version 0.2.0 downloaded
    run bresca updates status --format=json
    [[ $(jq -r .staged.download_only <<<"$output") == true ]]
    [[ $(jq -r .boots_next <<<"$output") == current ]]
    run bresca updates status
    [[ $output == *"(downloaded, not installed)"* ]]
}

@test "a queued rollback boots next" {
    keep_rollback_version 0.0.9
    change_state '.status.rollbackQueued = true'
    run bresca updates status --format=json
    [[ $(jq -r .rollback.version <<<"$output") == 0.0.9 ]]
    [[ $(jq -r .boots_next <<<"$output") == rollback ]]
}
