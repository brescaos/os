#!/usr/bin/env bats
# brew, Homebrew on demand: nothing here downloads or installs it. A fake
# Homebrew in tester's prefix stands for an installed one.

setup() {
    tester_prefix=/var/opt/$(id -u tester)/.linuxbrew
}

teardown() {
    rm -rf "/var/opt/$(id -u tester)"
}

# A fake brew for tester, printing its arguments.
install_fake_homebrew() {
    mkdir -p "$tester_prefix/bin"
    printf '#!/bin/sh\necho "fake brew: $*"\n' >"$tester_prefix/bin/brew"
    chmod 0755 "$tester_prefix/bin/brew"
    chown -R tester: "/var/opt/$(id -u tester)"
}

@test "brew refuses to run as root" {
    run brew --version
    [[ $status -eq 1 && $output == *"Homebrew doesn't run as root"* ]]
}

@test "brew runs the user's Homebrew with every argument" {
    install_fake_homebrew
    run runuser -u tester -- brew install "a b" c
    [[ $status -eq 0 && $output == "fake brew: install a b c" ]]
}

@test "brew installs nothing without a terminal" {
    run runuser -u tester -- brew --version </dev/null
    [[ $status -eq 1 && $output == *"run brew in a terminal to install it"* ]]
    [[ ! -e /var/opt/$(id -u tester) ]]
}

@test "the profile puts the user's Homebrew after the system's commands, once" {
    install_fake_homebrew
    run runuser -u tester -- sh -c '. /etc/profile.d/homebrew.sh; . /etc/profile.d/homebrew.sh; echo "$PATH"'
    [[ $output == *":$tester_prefix/bin:$tester_prefix/sbin" ]]
    [[ $output != *"$tester_prefix/bin"*"$tester_prefix/bin"* ]]
}

@test "the profile changes nothing without Homebrew" {
    run runuser -u tester -- sh -c 'before=$PATH; . /etc/profile.d/homebrew.sh; [ "$PATH" = "$before" ]'
    [[ $status -eq 0 ]]
}

@test "userdel removes the user's Homebrew" {
    local uid
    useradd leaving
    uid=$(id -u leaving)
    mkdir -p "/var/opt/$uid/.linuxbrew/bin"
    chown -R leaving: "/var/opt/$uid"
    userdel -r leaving
    [[ ! -e /var/opt/$uid ]]
}

@test "brew, its profile, its userdel hook and the git its installer needs are there" {
    [[ -x /usr/bin/brew && -f /etc/profile.d/homebrew.sh && -x /usr/libexec/bresca/homebrew/userdel ]]
    grep -qx 'USERDEL_CMD /usr/libexec/bresca/homebrew/userdel' /etc/login.defs
    [[ -f /usr/lib/systemd/system/accounts-daemon.service.d/50-brescaos-homebrew.conf ]]
    [[ -f /usr/lib/tmpfiles.d/homebrew.conf ]]
    command -v git
}

@test "the translations are there" {
    local lang
    for lang in es ca; do
        [[ -f /usr/share/locale/$lang/LC_MESSAGES/homebrew.mo ]] || {
            echo "missing: $lang"
            return 1
        }
    done
}

@test "brew's messages are in the user's language" {
    run env LANG=es_ES.UTF-8 runuser -u tester -- brew --version </dev/null
    [[ $output == *"Homebrew no está instalado"* ]]
}
