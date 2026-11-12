# bresca KRunner plugin

A KRunner plugin, **BrescaOS**: the `bresca` CLI's commands in KRunner
(Alt+Space) and in Plasma's search (the launcher). Today two, both in
System Settings > BrescaOS Updates (`image/src/updates-kcm/`):

| Command | Entry | What it does |
| --- | --- | --- |
| `bresca updates check` | **Check for BrescaOS updates** | Checks again, and shows the result |
| `bresca updates install` | **Install the new BrescaOS version** | The same, then downloads and installs the version it finds, or installs the one already downloaded |

| Source | In the image |
| --- | --- |
| `runner.cpp` and `krunner_brescaos.json.in`, translated | `/usr/lib64/qt6/plugins/kf6/krunner/krunner_brescaos.so` |
| `po/<lang>.po` | `/usr/share/locale/<lang>/LC_MESSAGES/krunner_brescaos.mo` |
| `krunnerrc` | `/etc/xdg/krunnerrc` |
| `com.brescaos.bresca-krunner.desktop.in`, translated | `/usr/share/applications/com.brescaos.bresca-krunner.desktop` |

Its version is in `VERSION.txt`: the build writes it into the plugin's
metadata.

It's C++ (KF6 Runner) because KRunner loads its plugins into its own
process: no service of its own runs for it. `image/Containerfile` compiles it
in its own stage, `bresca-krunner-build`, so the compilers and `-devel` packages
stay out of the image. Building it needs cmake, extra-cmake-modules,
gcc-c++, kf6-ki18n-devel and kf6-krunner-devel.

## How it works

- Each entry shows when every word typed is the start of the command's
  word in the same place, in any case: the whole command, while it's typed
  (`bresca updates inst`) or abbreviated (`b u i`, `bre upd ch`); both for
  their common start (`bre`, `b u`). The whole command comes first. It
  never runs what's typed.
- Its results come first: `krunnerrc` makes it KRunner's first favorite,
  ahead of applications (`krunner_services`, KRunner's own default). It only
  has results for the `bresca` commands, so it never gets ahead of anything
  else. A user's own `krunnerrc` (KRunner's settings) wins.
- **Ctrl+Alt+B** opens KRunner with only this plugin:
  `com.brescaos.bresca-krunner.desktop`, hidden from the menu, asks KRunner
  for it (`querySingleRunner`, with `gdbus`); kglobalaccel registers its
  `X-KDE-Shortcuts`, which System Settings > Keyboard > Shortcuts can
  change. There the commands work without `bresca` too (`updates ch`, `u c`), and
  anything else offers **“…” isn't a BrescaOS command: search all of
  KRunner**, which opens KRunner again with every plugin and what was
  typed, in case it was the wrong shortcut. Next to the search field,
  KRunner shows the plugin's icon. Its **?** types `?`, which lists every
  command: KRunner's own help plugin isn't there. It shows nothing until
  something is typed: KRunner doesn't ask a plugin for an empty query.
- Choosing one only opens the module with what to do:
  `systemsettings kcm_brescaos_updates --args check|install`. The module
  does it as its own buttons do, so it keeps their checks: polkit's
  password for downloading and installing, and the question before
  downloading from a registry without TLS.

To add an entry: a row in `commands` (`runner.cpp`) and a test in
`tests/runnertest.cpp`; what it does goes in the module.

## Translations

`kli18n()` and `i18n()` in the C++, domain `krunner_brescaos`
([`docs/translations.md`](../../../docs/translations.md)). The commands
aren't translated, as the CLI's.
`krunner_brescaos.json.in` is an English-only template: the build fills in
its translations and compiles it into the plugin.

## Tests

- `make check-bresca-krunner` (part of `make check`) compiles it with its
  tests: `tests/runnertest.cpp` loads the plugin as KRunner does and checks
  what each query finds.
- `make test` (on the host) checks in the image that the plugin, its
  translations, its shortcut and `krunnerrc` are installed
  (`tests/install-layout.bats`).

Choosing it only runs in the development VM.
