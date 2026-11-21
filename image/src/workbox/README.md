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

| Source | In the image |
| --- | --- |
| `core/`, `engines/podman/`, `cli/` | `/usr/bin/workbox` |
| `cli/completion.bash` | `/usr/share/bash-completion/completions/workbox` |
| `po/<lang>.po` | `/usr/share/locale/<lang>/LC_MESSAGES/workbox.mo` |
| `image/build/system.sh` | `/usr/share/workbox/workbox.conf` |

It's C++ (Qt, KF6 Config and I18n), so a KDE app to manage workboxes can use
the same code later. `image/Containerfile` compiles it in its own stage,
`workbox-build`. Building it needs cmake, extra-cmake-modules, gcc-c++,
qt6-qtbase-devel, kf6-kconfig-devel and kf6-ki18n-devel.

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

The workbox's root is root only inside it: rootless Podman maps it to the
user outside, so it can change anything of the workbox (its packages, its
home, what's shared with it), nothing else of the host.
- `workbox rm <id> [--yes]` stops it if it runs and removes it and its
  home volume, after asking to type its id, unless `--yes`.

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

- The colors (`core/terminalcolors.h`) take only the hue (OKLCH) of the
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

`/usr/share/workbox/workbox.conf`, written by `image/build/system.sh`,
follows the image; the user's `~/.config/workbox/workbox.conf` wins key by
key (KConfig, group `[General]`):

| Key | Image's value |
| --- | --- |
| `engine` | `podman`, the only one today |
| `image` | `registry.fedoraproject.org/fedora-toolbox:<Fedora release>`, Distrobox's too |

A new image applies to the workboxes created after it.

## How it works

One folder per layer; each is a static library the next one links, so the
tests (and later a KDE app) use the same code as the command line:

| Folder | Library | What it is |
| --- | --- | --- |
| `core/` | `workbox-core` | What no engine knows. `workbox.h`: the model, a workbox (id, name, color, and whether it's running, which only the engine knows), its labels and its volume's name. `color.h`: `--color`'s formats, the palette and the pick. `settings.h`: `workbox.conf`. `engine.h`: what workbox needs from an engine (list, create, start, stop, enter, remove). `terminalcolors.h`: the terminal's colors for a workbox. `commands.h`: what `create`, `list` and `rm` do, on any engine |
| `engines/podman/` | `workbox-podman` | `PodmanEngine`, the only code that knows Podman's commands. Another engine would go in `engines/<name>/` |
| `cli/` | (the program) | `main.cpp`, the command line: arguments, help, messages. `completion.bash`, its bash completion: keep it in sync with the commands and options; the ids come from `workbox list --quiet` |

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
- `enter` is `podman exec --interactive --tty --workdir $HOME --env TERM
  --env PS1=… --env WORKBOX_NAME=… <id> bash --login` (`--tty` only on a
  terminal), run on workbox's own terminal, so the shell reads and writes
  it; keep-id runs it as the user. `--root` adds `--user root` and
  `--workdir /root`.
- Podman's own errors go to stderr as they are; workbox adds which command
  failed.

## Translations

`i18n()` and `i18nc()` in the C++, domain `workbox`
([`docs/translations.md`](../../../docs/translations.md)). The commands and
options aren't translated; Qt's own words in the help (`Usage:`,
`Options:`) come from Qt's translations.

## Tests

- `make check-workbox` (part of `make check`) compiles it with its tests,
  each in `tests/` at its source's path (`tests/core/colortest.cpp` tests
  `core/color.cpp`): the model, the colors and the pick, the terminal's
  colors and their contrast, the settings, the commands on a fake engine, and `PodmanEngine`'s arguments with the fake
  `podman` of `tests/fakes/`.
- `make test` (on the host) checks in the image that `workbox` is
  installed and translated, that its settings name Distrobox's image
  and `create` uses it, and that bash completes its commands and `rm`'s
  ids (`tests/workbox.bats`, with the fake `podman`).

With the real Podman it only runs in the development VM.
