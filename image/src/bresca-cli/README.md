# bresca CLI

`bresca`, the BrescaOS system CLI. `bresca --help` lists its commands and
`bresca <command> --help` their options.

## Layout

| Source | In the image |
| --- | --- |
| `bresca` | `/usr/bin/bresca` |
| `commands/<command>` | `/usr/libexec/bresca/<command>` |
| `commands/<group>/<subcommand>` | `/usr/libexec/bresca/<group>/<subcommand>` |
| `completion.bash` | `/usr/share/bash-completion/completions/bresca` |
| `services/bresca-updates@.service` | `/usr/lib/systemd/system/bresca-updates@.service` |
| `services/bresca-updates-check.timer` | `/usr/lib/systemd/system/bresca-updates-check.timer` |
| `services/bresca-updates-check.rules` | `/usr/share/polkit-1/rules.d/50-brescaos-updates-check.rules` |
| `services/bresca-updates.tmpfiles` | `/usr/lib/tmpfiles.d/bresca-updates.conf` |
| `services/updates/<operation>` | `/usr/libexec/bresca/services/updates/<operation>` |
| `lib/<name>.sh` | `/usr/libexec/bresca/lib/<name>.sh`, sourced by commands |

`bresca` only dispatches: it runs `/usr/libexec/bresca/<command>` (or
`<group>/<subcommand>`, e.g. `updates/status`) with the remaining arguments.
Each command is its own script, with its own help and options. A group's help
(`bresca updates`) is in `bresca`.

## Adding a command

1. Add its script to `commands/` (or `commands/<group>/`).
2. Add it to the `case` and the usage text of `bresca` (or of its group). The
   `case` lists the commands on purpose, so `bresca` never runs an arbitrary
   path.
3. Add it and its options to `completion.bash`.

## Update services

Update operations don't run in the terminal or in System Settings: each one
runs as `bresca-updates@<operation>.service` (e.g. `bresca-updates@check`),
which runs `/usr/libexec/bresca/services/updates/<operation>`. So an operation
keeps going when whoever started it goes away, and a second `systemctl start`
waits for the running one instead of starting another.

Each operation writes its result to `/run/bresca/updates/<operation>.json`,
readable without root; it is removed when the operation starts and lasts
until the next reboot. Whether an operation is running is the unit's state
(`systemctl is-active bresca-updates@<operation>`). While it runs, an
operation may also publish its progress in
`/run/bresca/updates/<operation>-progress.json` (`download` does), removed
when it ends. The CLI commands only start the unit, show the progress and
print the result.

An operation can run others: `download-install` (`bresca updates install
--download`) runs `download` and then `install` in one unit, so the install
still happens if whoever started it goes away after the download; their
progress and results are the usual `download-progress.json`, `download.json`
and `install.json`.

`bresca updates registry` shows where updates come from: the followed image,
its registry, repository and tag, and whether the registry is allowed without
TLS. Every `bresca updates` command warns on stderr when the registry updates come
from is allowed without TLS (`insecure = true` in containers'
`registries.conf`), e.g. the development VM's local registry. The commands
that download (`download`, `install --download`) refuse it unless they're
given `--no-tls`. That check is the client's (the CLI, later the KCM): the
services don't make it, since `systemctl start` takes no parameters.

`rollback` deletes a staged version (downloaded, or installed and not booted
yet), as `bootc rollback` does, so it refuses unless it's given
`--discard-staged`; that check is the client's too.

`bresca-updates-check.timer` runs `bresca-updates@check` 5 minutes after boot
and every 6 hours: it only checks, and the tray applet
(`image/src/updates-applet/`) shows what it finds. `/run/bresca/updates/` is
created at boot (`bresca-updates.tmpfiles`) so the applet can watch it before
the first check. A successful install removes `check.json`: the version it
announced is installed.
`bresca-updates-check.rules` lets an administrator (group `wheel`) at the
computer start `bresca-updates@check` without a password, for the tray
applet's Check for Updates button: a check only reads. Nothing else.

To add an operation, add its script to `services/updates/`: the template unit
doesn't change.

## Tests

`tests/` holds bats tests that run in the BrescaOS image, with the CLI where
`image/Containerfile` installs it (`make test`, on the host; the CI runs them
too). `tests/Containerfile` adds bats and two fakes ahead in `PATH`:

- `fakes/bootc` keeps the system in a JSON file in `bootc status`'s shape,
  which each test writes with `helpers.bash`, and changes it as bootc would
  (`--check`, `--download-only`, `--from-downloaded`, `rollback`).
- `fakes/systemctl` runs an operation's real script on `start`, as
  `bresca-updates@.service` would, and records every call (so a test can
  check that nothing started, or that `reboot` was asked).

They cover where each file is installed, the dispatcher and the help, and
every `updates` command with its results and refusals (`--no-tls`,
`--discard-staged`). The fakes behave as these scripts expect bootc to: only
the VM shows that the real one does.
