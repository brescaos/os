# bresca KRunner plugin

A KRunner plugin, **BrescaOS**: the `bresca` CLI's commands in KRunner
(Alt+Space) and in Plasma's search (the launcher). Today only one:
**Check for BrescaOS updates**.

| Source | In the image |
| --- | --- |
| `runner.cpp` and `krunner_brescaos.json.in`, translated | `/usr/lib64/qt6/plugins/kf6/krunner/krunner_brescaos.so` |
| `po/<lang>.po` | `/usr/share/locale/<lang>/LC_MESSAGES/krunner_brescaos.mo` |
| `krunnerrc` | `/etc/xdg/krunnerrc` |

Its version is in `VERSION.txt`: the build writes it into the plugin's
metadata.

It's C++ (KF6 Runner) because KRunner loads its plugins into its own
process: no service of its own runs for it. `image/Containerfile` compiles it
in its own stage, `bresca-krunner-build`, so the compilers and `-devel` packages
stay out of the image. Building it needs cmake, extra-cmake-modules,
gcc-c++, kf6-ki18n-devel and kf6-krunner-devel.

## How it works

- **Check for BrescaOS updates** shows for `bresca updates check` and while
  it's typed (`bre`, `bresca upd`), in any case. It never runs what's typed.
- Its results come first: `krunnerrc` makes it KRunner's first favorite,
  ahead of applications (`krunner_services`, KRunner's own default). It only
  has results for the `bresca` commands, so it never gets ahead of anything
  else. A user's own `krunnerrc` (KRunner's settings) wins.
- Choosing it starts `bresca-updates@check` (`systemctl start --no-block`,
  as the tray applet's **Check for Updates**) and opens System Settings >
  BrescaOS Updates (`image/src/updates-kcm/`), which shows it running and
  then its result: up to date, a newer version to install, or bootc's error.
  An administrator at the computer needs no password
  (`bresca-updates.rules`, `image/src/bresca-cli/`); anyone else gets
  polkit's dialog.

To add an entry: its match in `match()`, what it does in `run()` (from
`match.id()` when there's more than one), and a test in `tests/runnertest.cpp`.

## Translations

`i18n()` in the C++, domain `krunner_brescaos`
([`docs/translations.md`](../../../docs/translations.md)). The commands
aren't translated, as the CLI's.
`krunner_brescaos.json.in` is an English-only template: the build fills in
its translations and compiles it into the plugin.

## Tests

- `make check-bresca-krunner` (part of `make check`) compiles it with its
  tests: `tests/runnertest.cpp` loads the plugin as KRunner does and checks
  what each query finds.
- `make test` (on the host) checks in the image that the plugin, its
  translations and `krunnerrc` are installed (`tests/install-layout.bats`).

Choosing it only runs in the development VM.
