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
