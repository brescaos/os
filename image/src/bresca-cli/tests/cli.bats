#!/usr/bin/env bats
# The dispatcher, the help and bresca version.

load helpers

@test "bresca, help, -h and --help show the help" {
    local arguments
    for arguments in "" help -h --help; do
        # shellcheck disable=SC2086 # "" means no argument
        run bresca $arguments
        [[ $status -eq 0 && $output == *"Usage: bresca <command>"* ]]
    done
}

@test "an unknown command shows the help and fails with 2" {
    run bresca nonexistent
    [[ $status -eq 2 && $output == *"Usage: bresca <command>"* ]]
}

@test "bresca updates shows its subcommands; an unknown one fails with 2" {
    run bresca updates
    [[ $status -eq 0 && $output == *"Subcommands:"* ]]
    run bresca updates nonexistent
    [[ $status -eq 2 ]]
}

@test "every updates subcommand has its help" {
    local subcommand
    for subcommand in status check download install rollback registry; do
        run bresca updates "$subcommand" --help
        [[ $status -eq 0 && $output == *"Usage: sudo bresca updates $subcommand"* ]]
    done
}

@test "an unknown option fails with 2" {
    run bresca updates status --nonexistent
    [[ $status -eq 2 && $output == *"unknown option: --nonexistent"* ]]
}

@test "bresca version prints the image's version" {
    local image_version
    image_version=$(. /usr/lib/os-release && echo "$IMAGE_VERSION")
    run bresca version
    [[ $output == "BrescaOS $image_version" ]]
    run bresca version --short
    [[ $output == "$image_version" ]]
    run bresca version --format=json
    [[ $(jq -r .version <<<"$output") == "$image_version" ]]
}
