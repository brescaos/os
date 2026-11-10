#!/usr/bin/env bats
# bresca updates status: the deployments, which boots next and the image
# updates come from; bresca-updates@status, its result for System Settings.

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

@test "the image updates come from, as bresca updates registry shows it" {
    allow_without_tls registry.brescaos.com
    run --separate-stderr bresca updates status --format=json
    [[ $(jq -c .registry <<<"$output") == $(bresca updates registry --format=json 2>/dev/null) ]]
    [[ $(jq -r .registry.tls <<<"$output") == false ]]
    run --separate-stderr bresca updates status
    [[ $output == *"Updates from"*"$IMAGE"*"(without TLS)"* ]]
}

@test "bresca-updates@status writes what status prints, readable without root" {
    stage_version 0.2.0 downloaded
    systemctl start bresca-updates@status.service
    [[ $(stat -c %a /run/bresca/updates/status.json) == 644 ]]
    [[ $(</run/bresca/updates/status.json) == $(bresca updates status --format=json 2>/dev/null) ]]
}
