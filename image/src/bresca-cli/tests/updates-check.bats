#!/usr/bin/env bats
# bresca updates check, through bresca-updates@check.

load helpers

@test "a newer version is available" {
    publish_version 0.2.0
    run bresca updates check
    [[ $status -eq 0 && $output == *"BrescaOS 0.2.0 is available (running 0.1.0): 288.2 MB to download."* ]]
    run bresca updates check --format=json
    [[ $(jq -r .update_available <<<"$output") == true ]]
    [[ $(jq -r .available_version <<<"$output") == 0.2.0 ]]
    [[ $(jq -r .download_size <<<"$output") == 288200000 ]]
}

@test "the download size in bootc's other units" {
    publish_version 0.2.0
    export BOOTC_ADDED_SIZE="1.9 GB"
    run bresca updates check --format=json
    [[ $(jq -r .download_size <<<"$output") == 1900000000 ]]
    export BOOTC_ADDED_SIZE="950 bytes"
    run bresca updates check --format=json
    [[ $(jq -r .download_size <<<"$output") == 950 ]]
}

@test "without the size from bootc, the version only" {
    publish_version 0.2.0
    export BOOTC_ADDED_SIZE=
    run bresca updates check
    [[ $status -eq 0 && $output == *"BrescaOS 0.2.0 is available (running 0.1.0)."* ]]
    run bresca updates check --format=json
    [[ $(jq -r .download_size <<<"$output") == null ]]
}

@test "the registry has the booted version: up to date" {
    publish_version 0.1.0
    run bresca updates check
    [[ $status -eq 0 && $output == *"BrescaOS is up to date (0.1.0)."* ]]
    run bresca updates check --format=json
    [[ $(jq -r .download_size <<<"$output") == null ]]
}

@test "the result is readable without root" {
    publish_version 0.2.0
    run bresca updates check
    [[ $(stat -c %a /run/bresca/updates/check.json) == 644 ]]
}

@test "a failed check fails with its error" {
    export BOOTC_FAILS=check
    run --separate-stderr bresca updates check
    [[ $status -eq 1 && $stderr == *"couldn't check for updates"*"connection refused"* ]]
}
