#!/usr/bin/env bats
# bresca updates download, through bresca-updates@download.

load helpers

@test "a newer version is downloaded, not installed" {
    publish_version 0.2.0
    run bresca updates download
    [[ $status -eq 0 && $output == *"Downloaded BrescaOS 0.2.0."*"restarting discards it"* ]]
    [[ $(staged_field image.version) == 0.2.0 && $(staged_field downloadOnly) == true ]]
    [[ $(jq -r .download_only /run/bresca/updates/download.json) == true ]]
    [[ ! -e /run/bresca/updates/download-progress.json ]]
}

@test "no newer version: nothing to download" {
    publish_version 0.1.0
    run bresca updates download
    [[ $status -eq 0 && $output == *"nothing to download"* ]]
    [[ $(staged_field image) == null ]]
}

@test "a failed download fails with its error" {
    publish_version 0.2.0
    export BOOTC_FAILS=download
    run --separate-stderr bresca updates download
    [[ $status -eq 1 && $stderr == *"couldn't download the update"* ]]
}

@test "a registry without TLS is refused without --no-tls, and nothing starts" {
    boot_version 0.1.0 10.0.2.2:5050/brescaos:0.1.0-x86_64
    allow_without_tls 10.0.2.2:5050
    publish_version 0.2.0
    run --separate-stderr bresca updates download
    [[ $status -eq 1 && $stderr == *"without TLS"*"--no-tls"* ]]
    [[ ! -s $SYSTEMCTL_LOG ]]
}

@test "--no-tls accepts a registry without TLS, with a warning" {
    boot_version 0.1.0 10.0.2.2:5050/brescaos:0.1.0-x86_64
    allow_without_tls 10.0.2.2:5050
    publish_version 0.2.0
    run --separate-stderr bresca updates download --no-tls
    [[ $status -eq 0 && $output == *"Downloaded BrescaOS 0.2.0."* ]]
    [[ $stderr == *"warning:"*"without TLS"* ]]
}
