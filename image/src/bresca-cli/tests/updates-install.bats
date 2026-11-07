#!/usr/bin/env bats
# bresca updates install, through bresca-updates@install and
# bresca-updates@download-install.

load helpers

@test "nothing downloaded: nothing to install, even with a newer version in the registry" {
    publish_version 0.2.0
    run bresca updates install
    [[ $status -eq 0 && $output == *"Nothing to install"* ]]
    [[ $(staged_field image) == null ]]
}

@test "the downloaded version is installed" {
    stage_version 0.2.0 downloaded
    run bresca updates install
    [[ $status -eq 0 && $output == *"Installed BrescaOS 0.2.0. Restart to use it"* ]]
    [[ $(staged_field downloadOnly) == false ]]
}

@test "installing removes the last check's result; nothing to install keeps it" {
    mkdir -p /run/bresca/updates
    echo '{"update_available": true, "available_version": "0.2.0"}' >/run/bresca/updates/check.json
    run bresca updates install
    [[ -e /run/bresca/updates/check.json ]]
    stage_version 0.2.0 downloaded
    run bresca updates install
    [[ ! -e /run/bresca/updates/check.json ]]
}

@test "an already installed version is reported as installed" {
    stage_version 0.2.0 installed
    run bresca updates install
    [[ $status -eq 0 && $output == *"Installed BrescaOS 0.2.0."* ]]
}

@test "--download downloads the newer version and installs it" {
    publish_version 0.2.0
    run bresca updates install --download
    [[ $status -eq 0 && $output == *"Installed BrescaOS 0.2.0."* ]]
    [[ $(staged_field image.version) == 0.2.0 && $(staged_field downloadOnly) == false ]]
}

@test "--download with no newer version: up to date" {
    publish_version 0.1.0
    run bresca updates install --download
    [[ $status -eq 0 && $output == *"up to date (0.1.0): nothing to install"* ]]
}

@test "--download fails with the download's error and leaves no install result" {
    publish_version 0.2.0
    export BOOTC_FAILS=download
    mkdir -p /run/bresca/updates
    echo '{"installed_version": "9.9.9"}' >/run/bresca/updates/install.json
    run --separate-stderr bresca updates install --download
    [[ $status -eq 1 && $stderr == *"couldn't download the update"* ]]
    [[ ! -e /run/bresca/updates/install.json ]]
}

@test "--reboot restarts only when something is installed" {
    run bresca updates install --reboot
    # Not "! grep": bash ignores a negated command for set -e, so it never fails.
    run grep -qx reboot "$SYSTEMCTL_LOG"
    [[ $status -ne 0 ]]
    stage_version 0.2.0 downloaded
    run bresca updates install --reboot
    [[ $output == *"Restarting..."* ]]
    grep -qx reboot "$SYSTEMCTL_LOG"
}

@test "without TLS, --download needs --no-tls; installing what's downloaded only warns" {
    boot_version 0.1.0 10.0.2.2:5050/brescaos:0.1.0-x86_64
    allow_without_tls 10.0.2.2:5050
    publish_version 0.2.0
    run --separate-stderr bresca updates install --download
    [[ $status -eq 1 && $stderr == *"--no-tls"* ]]
    [[ ! -s $SYSTEMCTL_LOG ]]
    stage_version 0.2.0 downloaded
    run --separate-stderr bresca updates install
    [[ $status -eq 0 && $stderr == *"warning:"* && $output == *"Installed BrescaOS 0.2.0."* ]]
}
