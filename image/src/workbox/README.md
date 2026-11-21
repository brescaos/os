# workbox

`workbox` runs rootless Podman containers, each a separate place to work (for
example one per client), that share nothing with the host: no home folder, no
SSH agent, no display, no host network. Distrobox does the opposite. Today it
creates, lists, starts, stops, enters and removes them; the options that
share something with the host come next.

```bash
workbox create c1 --name "Cliente 01" --color '#3584e4'
workbox list
workbox start c1
workbox enter c1
workbox stop c1
workbox rm c1
```

`workbox --help` lists the commands, and `workbox <command> --help` shows
that command's options, with an example. Bash completes the commands,
their options and the ids `start`, `stop`, `enter` and `rm` take.

| Source | Installed as |
| --- | --- |
| `lib/`, `cli/` | `/usr/bin/workbox` |
| `cli/completion.bash` | `/usr/share/bash-completion/completions/workbox` |
| `config/workbox.conf` | `/usr/share/workbox/workbox.conf` |
| `po/<lang>.po` | `/usr/share/locale/<lang>/LC_MESSAGES/workbox.mo` |

Its version is in `VERSION.txt`; `workbox --version` shows it.

It's C++ (Qt, KF6 Config, CoreAddons and I18n): a library, the command
line on it, and later an applet and an app on the same library. It runs
`podman` (rootless) and, for workboxes, Fedora's toolbox image. Building it
needs cmake, extra-cmake-modules, gcc-c++, qt6-qtbase-devel,
kf6-kconfig-devel, kf6-kcoreaddons-devel and kf6-ki18n-devel:

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
cmake --install build
```

## Commands

- `workbox create <id> [--name <name>] [--color <color>] [--sudo]` creates
  the container and its home volume, `workbox-<id>-home`, mounted at the
  same path as the user's home. The image is downloaded the first time.
  Inside, the user is the user outside (keep-id), not root. `--sudo` lets
  them use sudo in it without a password, to install packages with dnf:
  whatever runs in it as the user can then be its root.
- `workbox list [--quiet]` shows every workbox by id, with its name
  and its status.
- `workbox start <id>` and `workbox stop <id>` start and stop it, and say
  so when it already is (not an error). Stopping it stops everything in it.
- `workbox enter <id> [--root]` opens a login bash in it, in the user's
  home, starting it first if it's stopped (see [Terminal](#terminal)). Its
  exit status is the shell's. `--root`, as the workbox's root in `/root`,
  with or without `--sudo`.
- `workbox rm <id> [--yes]` stops it if it runs and removes it and its
  home volume, after asking to type its id, unless `--yes`.

The workbox's root is root only inside it: rootless Podman maps it to the
user outside, so it can change anything of the workbox (its packages, its
home, what's shared with it), nothing else of the host.

The **id** (`c1`) is what every command takes: `[a-z0-9][a-z0-9-]*`, up to
32 characters, unique, and never changes. The **name** is only shown
(spaces, accents, the same in two workboxes); without `--name`, it's the id.

### Color

Every workbox has a color, to tell it apart, and its terminal's hue.
`--color` takes `#rgb`, `#rrggbb` or `r,g,b` (each 0–255), kept as
`#rrggbb`. Without it, workbox picks one of its palette: 32 hues evenly
spaced in OKLCH from 0°, all with lightness 0.7 and chroma 0.11 (every hue
has them in sRGB). It picks, among the colors no workbox uses, the one
whose nearest color in use is the farthest away in OKLab, so each new one
looks as different as possible from the others (a color given with
`--color` counts too); the first in the palette's order on a tie, so the
first workbox gets the first color. With all 32 in use, the least used one.

### Terminal

In a terminal, `workbox enter` turns the background a dark shade of the
workbox's hue, with light text, and the prompt shows a toolbox and the
workbox's name, bold in a light shade of that hue:

```
🧰 Cliente 01 [user@c1 ~]$
```

- The colors (`lib/core/terminalcolors.h`) take only the hue (OKLCH) of the
  workbox's color: the background at lightness 0.28 and chroma 0.048 (what
  sRGB has for every hue), the text `#fcfcfc`, the label at 0.84 with up to
  0.11 chroma. Text and label read at 7:1 or more (WCAG AAA) on every
  background, also for any `--color`; the tests check it every 0.5°.
- `workbox` sets them with OSC 11 and 10 before the shell, and puts the
  terminal profile's back with OSC 111 and 110 when it ends, however it
  ends (Konsole ignores them without a `;`). Not in a pipe or a file.
- The prompt goes in as `PS1`, which Fedora's `/etc/bashrc` keeps; the name
  in `WORKBOX_NAME`, which `PS1` expands, so what's in a name (`$(…)`,
  backslashes) is shown, never run. A `PS1` set in the workbox's
  `~/.bashrc` wins; zsh and fish don't show it.
- 🧰 is an emoji: it shows with the terminal's emoji font.

## Settings

`workbox.conf` (KConfig, group `[General]`), on three levels, each key from
the most specific that has it:

| File | Whose |
| --- | --- |
| `/usr/share/workbox/workbox.conf` | workbox's own (`config/workbox.conf`) |
| `/usr/share/defaults/workbox/workbox.conf` | a distribution's, if it has one (as Distrobox's, in `/usr/share/defaults/distrobox/`) |
| `~/.config/workbox/workbox.conf` | the user's |

| Key | workbox's value |
| --- | --- |
| `engine` | `podman`, the only one today |
| `image` | `registry.fedoraproject.org/fedora-toolbox:latest`; a distribution can name its own release's |

A new image applies to the workboxes created after it. The paths follow the
install prefix (`<datadir>/workbox/`, `<datadir>/defaults/workbox/`).

## How it works

Three parts: the library (`lib/`), the command line (`cli/`) and, later, an
applet and an app. The library is static, one folder per layer, each a
library the next one links, so the tests and every part use the same code.

**Nothing blocks.** Every engine operation is a job, an
`EngineJob` (`lib/core/enginejob.h`): a `KJob` that runs without waiting and
says when it's done with its `result()` signal, so an applet or an app can
use the engine without freezing. On failure, its `errorText()` says why,
translated. Its `output(line)` signal gives what the engine prints as it
goes (Podman's standard error: an image's download, its errors), for
whoever wants to show it. The command line runs each job with `exec()`,
which waits in an event loop, and prints its output to stderr as it comes.
`enter`'s job (`EnterJob`) runs the shell on the process's terminal, which
podman reads and writes itself, and ends when the shell exits, with its
exit status: the shell failing isn't the job failing.

**Following changes.** `Engine::watch()`'s job (`WatchJob`) runs until it's
killed and emits `changed()` when a workbox may have changed, wherever it
changed (workbox, a terminal, another program), without polling. It doesn't
say what changed: whoever follows lists again. Several changes in a row (one
operation makes a few) give one `changed()`, 100 ms after the last. Start it
before the first `list()`, so no change between them is missed.

| Folder | Library | What it is |
| --- | --- | --- |
| `lib/core/` | `workbox-core` | What no engine knows. `workbox.h`: the model, a workbox (id, name, color, and whether it's running, which only the engine knows), its labels and its volume's name. `color.h`: `--color`'s formats, the palette and the pick. `settings.h`: `workbox.conf`. `enginejob.h`: an engine's job. `engine.h`: what workbox needs from an engine (list, watch, create, start, stop, enter, remove, each a job). `terminalcolors.h`: the terminal's colors for a workbox. `commands.h`: what the commands do on any engine: `CreateJob` (lists, checks the id, picks the color, creates), `find` and `sortedById` |
| `lib/engines/podman/` | `workbox-podman` | `PodmanEngine`, the only code that knows Podman's commands, and `PodmanJob`, which runs them one after another with `QProcess`'s signals (with cleanup commands to undo the earlier ones when one fails, never when the first one does), `PodmanEnterJob`, `podman exec`'s shell on the terminal, and `PodmanWatchJob`, `podman events`. Another engine would go in `lib/engines/<name>/` |
| `cli/` | (the program) | `main.cpp`, the command line: arguments, help, messages, `--version`. `completion.bash`, its bash completion: keep it in sync with the commands and options; the ids come from `workbox list --quiet` |

- A workbox is a container with the labels `com.brescaos.workbox.name`,
  `com.brescaos.workbox.color` and, with `--sudo`,
  `com.brescaos.workbox.sudo=true`, the only copy of its settings; the
  name's label is how workbox finds its containers.
- `podman create --userns=keep-id --network=pasta` with its labels and
  `--volume workbox-<id>-home:$HOME`, running `sleep infinity`: the same uid
  inside, its own network (outgoing only), nothing of the host shared.
- With `--sudo`, `create` starts it, its root writes
  `#<uid> ALL=(ALL) NOPASSWD: ALL` to `/etc/sudoers.d/workbox` (mode 0440)
  and stops it. A file mounted from the host wouldn't be owned by the
  workbox's root, and sudo would refuse it. If that fails, it removes the
  workbox: nothing is left half done.
- `start` is `podman start`; `stop` is `podman stop --time 0`, and `rm` is
  `podman rm --force --time 0` then `podman volume rm` (sleep ignores
  SIGTERM, so Podman would wait 10 seconds to kill it anyway).
- `watch` is `podman events --format json --filter type=container --filter
  label=com.brescaos.workbox.name`, running all along: each line it prints
  is a change. Removing a workbox's volume has no label, but its container's
  removal is an event already. If `podman events` ends (Podman updated, a
  crash), it's started again 10 seconds later, and that's a change too,
  since some may have been missed.
- `enter` is `podman exec --interactive --tty --workdir $HOME --env TERM
  --env PS1=… --env WORKBOX_NAME=… <id> bash --login` (`--tty` only on a
  terminal), run on workbox's own terminal, so the shell reads and writes
  it; keep-id runs it as the user. `--root` adds `--user root` and
  `--workdir /root`.
- Podman's standard error is the job's output, so on the command line it
  shows as it did; the job's error says which command failed.

## Translations

`i18n()` and `i18nc()` in the C++, domain `workbox`: `po/<lang>.po`, for
now with the rest of the repository's
([`docs/translations.md`](../../../docs/translations.md)). The commands and
options aren't translated; Qt's own words in the help (`Usage:`,
`Options:`) come from Qt's translations.

## Tests

- With `-DBUILD_TESTING=ON`, `ctest` runs its tests (`make check-workbox`
  in this repository), each in `tests/` at its source's path
  (`tests/lib/core/colortest.cpp` tests `lib/core/color.cpp`): the model, the colors and the pick, the terminal's
  colors and their contrast, the settings, the commands on a fake engine,
  and `PodmanEngine`'s arguments with the fake `podman` of `tests/fakes/`.
  They also check that nothing blocks: results come after `start()`
  returns, and the event loop keeps running while podman does; a job's
  output, and killing it; that several events are one change, and that
  `podman events` is started again when it ends.
- `tests/cli/workbox.bats` checks an installed workbox, for now in this
  repository's image (`tests/Containerfile`, `make test`): its own settings,
  that `create` takes the image the user's settings name over the others,
  `--version`, its translations and its bash completion, with the fake
  `podman`.

Neither runs the real Podman.
