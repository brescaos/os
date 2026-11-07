#!/usr/bin/env bats
# bresca updates registry: the followed image's parts, and registries.conf.

load helpers

# Without --separate-stderr, run puts the TLS warning in $output, before the JSON.
registry_field() {
    run --separate-stderr bresca updates registry --format=json
    jq -r ".$1" <<<"$output"
}

@test "an image with a registry, a repository of two levels and a tag" {
    boot_version 0.1.0 registry.brescaos.com/brescaos/brescaos:0.1.0-x86_64
    [[ $(registry_field registry) == registry.brescaos.com ]]
    [[ $(registry_field repository) == brescaos/brescaos ]]
    [[ $(registry_field tag) == 0.1.0-x86_64 ]]
    [[ $(registry_field tls) == true ]]
}

@test "a registry's port isn't a tag; no tag means latest" {
    boot_version 0.1.0 10.0.2.2:5050/brescaos
    [[ $(registry_field registry) == 10.0.2.2:5050 ]]
    [[ $(registry_field repository) == brescaos ]]
    [[ $(registry_field tag) == latest ]]
}

@test "insecure = true for the registry means no TLS, with a warning" {
    boot_version 0.1.0 10.0.2.2:5050/brescaos:0.0.0-x86_64
    allow_without_tls 10.0.2.2:5050
    [[ $(registry_field tls) == false ]]
    run --separate-stderr bresca updates registry
    [[ $output == *"(without TLS)"* && $stderr == *"warning:"* ]]
}

@test "registries.conf: single quotes and indentation count" {
    boot_version 0.1.0 10.0.2.2:5050/brescaos:0.0.0-x86_64
    mkdir -p /etc/containers/registries.conf.d
    printf "[[registry]]\n  location = '10.0.2.2:5050'  \n  insecure = true\n" >/etc/containers/registries.conf.d/50-tests.conf
    [[ $(registry_field tls) == false ]]
}

@test "registries.conf: insecure = false, or another table's insecure, keeps TLS" {
    boot_version 0.1.0 10.0.2.2:5050/brescaos:0.0.0-x86_64
    mkdir -p /etc/containers/registries.conf.d
    printf '[[registry]]\nlocation = "10.0.2.2:5050"\ninsecure = false\n\n[[registry]]\nlocation = "other:5000"\ninsecure = true\n' \
        >/etc/containers/registries.conf.d/50-tests.conf
    [[ $(registry_field tls) == true ]]
}
