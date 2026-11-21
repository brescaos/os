# workbox

`workbox` runs rootless Podman containers, each a separate place to work (for
example one per client), that share nothing with the host: no home folder, no
SSH agent, no display, no host network. Distrobox does the opposite. Today it
creates, lists and removes them; entering them and the options that share something
with the host come next.

```bash
workbox create c1 --name "Cliente 01" --color '#3584e4'
workbox list
workbox rm c1
```

`workbox --help` lists the commands, and `workbox <command> --help` shows
that command's options, with an example. Bash completes the commands,
their options and `rm`'s ids.

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

- `workbox create <id> [--name <name>] [--color <color>]` creates the
  container and its home volume, `workbox-<id>-home`, mounted at the same
  path as the user's home. The image is downloaded the first time.
- `workbox list [--quiet]` shows every workbox by id, with its name and
  color, in columns; `--quiet` (`-q`), only the ids, one per line, for
  scripts and the completion.
- `workbox rm <id> [--yes]` stops it if it runs and removes it and its
  home volume, after asking (`[y/N]`) unless `--yes`.

The **id** (`c1`) is what every command takes: `[a-z0-9][a-z0-9-]*`, up to
32 characters, unique, and never changes. The **name** is only shown
(spaces, accents, the same in two workboxes); without `--name`, it's the id.

### Color

Every workbox has a color, to tell it apart (and later, its terminal's).
`--color` takes `#rgb`, `#rrggbb` or `r,g,b` (each 0–255), kept as
`#rrggbb`. Without it, workbox picks one of its palette: 32 hues evenly
spaced in OKLCH from 0°, all with lightness 0.7 and chroma 0.11 (every hue
has them in sRGB). It picks, among the colors no workbox uses, the one
whose nearest color in use is the farthest away in OKLab, so each new one
looks as different as possible from the others (a color given with
`--color` counts too); the first in the palette's order on a tie, so the
first workbox gets the first color. With all 32 in use, the least used one.

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
| `core/` | `workbox-core` | What no engine knows. `workbox.h`: the model, a workbox (id, name, color), its labels and its volume's name. `color.h`: `--color`'s formats, the palette and the pick. `settings.h`: `workbox.conf`. `engine.h`: what workbox needs from an engine (list, create, remove). `commands.h`: what `create`, `list` and `rm` do, on any engine |
| `engines/podman/` | `workbox-podman` | `PodmanEngine`, the only code that knows Podman's commands. Another engine would go in `engines/<name>/` |
| `cli/` | (the program) | `main.cpp`, the command line: arguments, help, messages. `completion.bash`, its bash completion: keep it in sync with the commands and options; `rm`'s ids come from `workbox list --quiet` |

- A workbox is a container with the labels `com.brescaos.workbox.name` and
  `com.brescaos.workbox.color`, the only copy of its settings; the name's
  label is how workbox finds its containers.
- `podman create --userns=keep-id --network=pasta` with its labels and
  `--volume workbox-<id>-home:$HOME`, running `sleep infinity`: the same uid
  inside, its own network (outgoing only), nothing of the host shared.
- `rm` is `podman rm --force --time 0` (sleep ignores SIGTERM, so Podman
  would wait 10 seconds to kill it anyway), then `podman volume rm`.
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
  `core/color.cpp`): the model, the colors and the pick, the settings, the
  commands on a fake engine, and `PodmanEngine`'s arguments with the fake
  `podman` of `tests/fakes/`.
- `make test` (on the host) checks in the image that `workbox` is
  installed and translated, that its settings name Distrobox's image
  and `create` uses it, and that bash completes its commands and `rm`'s
  ids (`tests/workbox.bats`, with the fake `podman`).

With the real Podman it only runs in the development VM.
