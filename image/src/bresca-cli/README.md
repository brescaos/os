# bresca CLI

`bresca`, the BrescaOS system CLI. `bresca --help` lists its commands and
`bresca <command> --help` their options.

## Layout

| Source | In the image |
| --- | --- |
| `bresca` | `/usr/bin/bresca` |
| `commands/<command>` | `/usr/libexec/bresca/<command>` |
| `completion.bash` | `/usr/share/bash-completion/completions/bresca` |

`bresca` only dispatches: it runs `/usr/libexec/bresca/<command>` with the
remaining arguments. Each command is its own script, with its own help and
options.

## Adding a command

1. Add its script to `commands/`.
2. Add it to the `case` and the usage text of `bresca`. The `case` lists the
   commands on purpose, so `bresca` never runs an arbitrary path.
3. Add it and its options to `completion.bash`.
