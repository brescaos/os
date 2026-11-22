# workbox

`workbox` runs rootless Podman containers, each a separate place to work (for
example one per client), that share nothing with the host but the folders
they're given: no home folder, no SSH agent, no display, no host network.
Distrobox does the opposite. Today it creates, lists, starts, stops, enters
and removes them, from a terminal or its app (a window with their terminals,
and its icon in the system tray); the other options that share something with the host
come next.

```bash
workbox create c1 --name "Cliente 01" --color '#3584e4'
workbox list
workbox info c1
workbox start c1
workbox enter c1
workbox stop c1
workbox rm c1
```

`workbox --help` lists the commands, and `workbox <command> --help` shows
that command's options, with an example. Bash completes the commands,
their options and the ids `info`, `start`, `stop`, `enter` and `rm` take.

| Source | Installed as |
| --- | --- |
| `lib/`, `cli/` | `/usr/bin/workbox` |
| `cli/completion.bash` | `/usr/share/bash-completion/completions/workbox` |
| `config/workbox.conf` | `/usr/share/workbox/workbox.conf` |
| `app/workboxmodel*`, `app/window/` | `/usr/bin/workbox-app` |
| `app/com.brescaos.workbox.desktop.in` (translated) | `/usr/share/applications/com.brescaos.workbox.desktop` |
| `app/icons/com.brescaos.workbox.svg` | `/usr/share/icons/hicolor/scalable/apps/com.brescaos.workbox.svg`, the app's icon (its `.desktop`, its window, no workboxes) |
| `app/icons/workbox-symbolic.svg` | `/usr/share/icons/hicolor/scalable/apps/com.brescaos.workbox-symbolic.svg`, the toolbox of the tray icon and of each workbox's color in the window |
| `app/icons/workbox-running-symbolic.svg` | `/usr/share/icons/hicolor/scalable/apps/com.brescaos.workbox-running-symbolic.svg`, the tray icon while any workbox runs: the toolbox with a dot |
| `po/<lang>.po` | `/usr/share/locale/<lang>/LC_MESSAGES/workbox.mo` |

Its version is in `VERSION.txt`; `workbox --version` shows it.

It's C++ (Qt, KF6 Config, CoreAddons and I18n): a library, and the command
line and an app on it (Qt Widgets, KF6 Parts, StatusNotifierItem,
WidgetsAddons and ColorScheme). It runs `podman` (rootless) and, for
workboxes, Fedora's toolbox image; the app needs Konsole's part
(`konsolepart`, Fedora's `konsole-part`) for its terminals, and its tray
icon a system tray with StatusNotifierItems (Plasma's). Building it needs
cmake, extra-cmake-modules, gcc-c++, qt6-qtbase-devel, kf6-kconfig-devel,
kf6-kcoreaddons-devel, kf6-ki18n-devel, kf6-kparts-devel (which brings
WidgetsAddons' and ColorScheme's) and kf6-kstatusnotifieritem-devel, and the
app's `com.brescaos.workbox.desktop` with its translations, from
`app/com.brescaos.workbox.desktop.in` (this repository's
`image/build/compile_translations.sh` writes it):

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr -DTRANSLATED_METADATA_DIR=<folder with the .desktop>
cmake --build build
cmake --install build
```

## Commands

- `workbox create <id> [--name <name>] [--color <color>] [--sudo]
  [-v [<folder>|<name>:]<target>[:<options>]]… [--wayland]` creates the container and
  its home, the volume `workbox-<id>.home`, mounted at the same path as the
  user's home. The
  image is downloaded the first time. `-v` gives it a folder of the
  user's home, or volumes of its own (see [Volumes](#volumes)).
  Inside, the user is the user outside (keep-id), not root. `--sudo` lets
  them use sudo in it without a password, to install packages with dnf:
  whatever runs in it as the user can then be its root. `--wayland` lets
  its apps show windows (see [Wayland](#wayland)).
- `workbox list [--quiet]` shows every workbox by id, with its name
  and its status.
- `workbox info <id>` shows it: id, name, color, status, image, whether
  it has `--sudo` and `--wayland`, its home and its volumes. In a terminal, the id in its color
  and the status in green or red, as `list`.
- `workbox start <id>` and `workbox stop <id>` start and stop it, and say
  so when it already is (not an error). Stopping it stops everything in it.
  `workbox stop --all` stops every running one.
- `workbox enter <id> [--root] [-- <command>…]` opens a login bash in it,
  in the user's home, starting it first if it's stopped (see
  [Terminal](#terminal)), or runs the command after `--`: straight, its
  arguments as they are (no shell reads them), without the login shell's
  profile nor the terminal's colors, as Toolbox and Distrobox do, but none
  of the host's environment but `TERM`. Its exit status is the shell's or
  the command's. `--root`, as the workbox's root in `/root`, with or
  without `--sudo`:

  ```bash
  workbox enter c1 -- make -j4
  workbox enter c1 --root -- dnf -y install htop
  ```
- `workbox rm <id>|--all [--yes]` stops it if it runs and removes it and its
  volumes (its home too), after asking to type its id, unless `--yes`.
  `--all` removes every workbox, after asking `[y/N]` with their ids,
  unless `--yes`.

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

## App

`workbox-app` (Workbox, in the apps' menu) shows every workbox on the left,
by id, with its color, name, id and status, how many run, and a search by
name or id; the first one is selected. On the right, the selected one:

- Its color, name, id and status, and its buttons: **Start**, or **Stop**
  and **Open in Konsole** (a shell in it in a Konsole window of its own)
  while it runs, and **Remove**, which asks to type its id first, as
  `workbox rm`. While one of them is under way, they're disabled. A failure shows
  over its tabs, with the job's error, until its next operation.
- **Terminal**: its shells, each a tab of Konsole's part, in its terminal
  colors and with its prompt, as `workbox enter` (see [Terminal](#terminal)),
  and **+** for another. The first time it's shown running, and when it's
  started from the window, a shell opens in it; a tab goes when its shell
  ends (`exit`, or the workbox stopped). Stopped, **Start and Open a
  Terminal**. Each workbox keeps its shells while another is selected.
- **Information**: its image, home, color, sudo, Wayland and volumes, as
  `workbox info`.

**Create** opens its dialog, as `workbox create`: its name, and its id,
which follows the name (lowercase, without accents) until one is typed and
says when it's invalid or taken; the image `workbox.conf` names; its color,
at first the one `create` would pick, **Change…** for any other, and under
it five suggestions, smaller, to click: that one and what `create` would
pick after it, each with the ones before in use too
(`WorkboxColor::suggestions()`); its
volumes, a row each (where it is from and in the workbox, its type, and
buttons to edit and remove it), and **Add Volume…**, which opens a dialog
of its own (`dialogs/volume/add.h`, also to edit one): its type, a shared
volume (a folder of the home, typed with `~` for the home or chosen, and
read-only), a labeled one (a named volume of its own) or a volume of its
own without a name; then where, its source → the folder in the workbox,
which a shared folder fills in with its own place in the home while it's
empty; `-v`'s error in place of its hint, and Add only once it can be; and
Wayland and sudo. Create is enabled only once it can be. Then its progress,
with the engine's last line (an image's download) and Cancel; a failure goes
back to the form with why. Once created, it's selected, and the dialog offers
**Open a Terminal** in it. With no workboxes, the window says what one is;
when workbox can't run (its settings, its engine), why, and Create is
disabled.

The terminal is Konsole's part, which runs `sh` to set the colors (OSC 11
and 10, as `workbox enter` does) and then `Engine::shellCommandLine()`'s
command, `podman exec --tty … bash --login`, with its prompt in `PS1` and
`WORKBOX_NAME`: the part is the terminal. **Open in Konsole** runs `konsole
-e` with the same command. Nothing blocks: every operation is the list's
(`WorkboxModel`) job.

### Tray icon

While the app runs, and only then, the system tray shows its icon: the
toolbox (`com.brescaos.workbox-symbolic`) or, while any workbox runs, the
toolbox with a dot on its corner (`com.brescaos.workbox-running-symbolic`,
the dot in the color scheme's highlight); its tooltip says how many run. It
goes when the app quits, with its window. It's a StatusNotifierItem
(`KStatusNotifierItem`, `app/window/trayicon.h`). Both icons go by name and
follow the color scheme, so the tray colors them as its other symbols: a
pixmap, or an overlay (which Plasma draws onto the icon), it would draw in
their fallback colors instead, dark on a dark panel.

- A click shows the window, and raises it.
- Its menu has a submenu per workbox, by id, with its color and its status
  in a tooltip: **Start** or **Stop**, and **Open a Terminal**, which shows
  the window on its terminals with a new shell, starting it first if it's
  stopped. Both are disabled while an operation is under way. Removing is
  only in the window, which asks to type the id. Then **Quit**. With no
  workboxes, or when workbox can't run, it says so.

**The list (`WorkboxModel`, `app/workboxmodel.h`)** is the app's, the
window's and the tray icon's, on the library: the engine `workbox.conf` names,
its `watch()` started before the first `list()`, and a list again on each
change, wherever it changed (the app, a terminal, another program). Its rows
change one by one, so a row asking to remove keeps asking while others
change. Nothing blocks the app: every operation is the engine's job.

### Volumes

`-v`/`--volume [<folder>|<name>:]<target>[:<options>]` works as Podman's
`-v`, with workbox's rules:

- `-v <folder>:<target>` mounts a folder of the user's home at `target`;
- `-v <name>:<target>` gives the workbox a volume of its own, Podman's
  `workbox-<id>.<name>` (created if it's missing): the name is only the
  workbox's, so a workbox can't reach another's volumes, its home among
  them, nor other programs'. `[a-z0-9][a-z0-9-]*`, up to 32 characters, and
  not `home`, its home's;
- `-v <target>` alone gives the workbox an unnamed one there (Podman's
  anonymous volume);
- options, comma-separated: `ro` (read-only) or `rw`; `z` is accepted, as
  workbox always adds it. Everything else is refused: `Z` would take the
  folder from other workboxes that share it, `U` would change the owner of
  the host's files, and the rest aren't needed yet;

`target` is a folder of the workbox's home, absolute or `~/…` (workbox
expands the `~`, which the shell doesn't after a `:`), never the home
itself, whose volume it would hide, without `..`, `:` or `,`; two volumes
can't have the same target, but the same folder can be at two. Several
workboxes can share the same folder. Only when it's created, for now:
changing them means creating the container again. A workbox's volumes,
named or not, its home too, are removed with it. `info` shows
`<folder> → <target>`, `<name> → <target> (volume)` or `<target> (its own)`:

```bash
workbox create c1 -v ~/Clients/01:~/project -v ~/.config/gh:~/.config/gh:ro -v cache:~/.cache -v ~/tmp
```

Every volume of a workbox is `workbox-<id>.<name>`, its home
`workbox-<id>.home`: ids have no dots, so two workboxes' volumes never have
the same name.

`lib/core/volumes.h` decides which folders can be shared, at their real path
(links resolved, so a link can't get round it), and refuses:

- what isn't a folder, and what's outside the home folder;
- the home folder itself, and any folder that holds a blocked one;
- keys, `~/.ssh` and `~/.gnupg` (the SSH agent will be the way to use them);
- what the host runs code from, where a workbox that can write would run
  code on the host at the next login: `~/.config/autostart`,
  `~/.config/systemd`, `~/.config/environment.d`,
  `~/.config/plasma-workspace`, `~/.local/bin`,
  `~/.local/share/applications`, `~/.local/share/kservices5`,
  `~/.local/share/kservices6` and `~/.local/share/plasma` (the shell's own
  files, `~/.bashrc` and the like, are in the home folder itself);
- `~/.config` and `~/.local` whole (other apps' settings and tokens), and
  their other folders unless read-only (`~/.config/gh:ro`);
- paths with `:` or `,`, which Podman's `--volume` can't take.

**SELinux.** A container (`container_t`) can't read the files of a home
folder (`user_home_t`), so a folder mounted into it stays unreadable unless
it's relabeled. workbox mounts each one with Podman's `z`, which relabels it
on the host as `container_file_t`, readable by containers; the host's apps
use it as before. Not `Z`, which labels a folder for one container only, so
a second workbox sharing it would take it from the first. Removing a
workbox doesn't relabel its folders back: `restorecon -R <folder>` does.
What isn't shared keeps its label, so the workbox can't read it, even if a
process escapes the container. Without SELinux, `z` does nothing.

### Wayland

`--wayland` lets the workbox's apps show windows in the user's Wayland
session, with the GPU, as Flatpak's `--socket=wayland` and `--device=dri`:

```bash
workbox create c1 --wayland
workbox enter c1 -- dolphin
```

- Only the session's socket (`$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY`), not the
  user's whole runtime folder, where PipeWire, the session bus and the SSH
  agent are too: it's at `/run/workbox/wayland-0`, which `WAYLAND_DISPLAY`
  names (libwayland takes an absolute path).
- A runtime folder of its own, `/run/user/<uid>` (a `tmpfs`, the user's,
  0700), as Qt and GTK want one; the GPU (`/dev/dri`); and
  `QT_QPA_PLATFORM=wayland`, as there's no X11.
- What it opens: windows and the GPU. Wayland keeps them from reading other
  apps' windows and keyboard; an app of the workbox can read the clipboard
  while it has the focus, as any app.
- `create --wayland` needs a Wayland session (`WAYLAND_DISPLAY`). Podman
  mounts the socket when the workbox starts, so after a new login it's the
  new session's; if KWin restarts while the workbox runs, its apps can't
  open windows until it's stopped and started again.
- **SELinux (not checked yet).** The socket is mounted with `z`, as a
  shared folder, so a confined workbox (`container_t`) can use it; whether
  that's enough to connect to KWin (`unconfined_t`) only shows on an
  installed system. If it isn't, it needs a policy of its own or
  `--unconfined`.

## Settings

`workbox.conf` (KConfig, group `[General]`), on three levels, each key from
the most specific that has it:

| File | Whose |
| --- | --- |
| `/usr/share/workbox/workbox.conf` | workbox's own (`config/workbox.conf`) |
| `/usr/share/defaults/workbox/workbox.conf` | a distribution's, if it has one |
| `~/.config/workbox/workbox.conf` | the user's |

| Key | workbox's value |
| --- | --- |
| `engine` | `podman`, the only one today |
| `image` | `registry.fedoraproject.org/fedora-toolbox:latest`; a distribution can name its own release's |

A new image applies to the workboxes created after it. The paths follow the
install prefix (`<datadir>/workbox/`, `<datadir>/defaults/workbox/`).

## How it works

Three parts: the library (`lib/`), the command line (`cli/`) and the app
(`app/`: its window and its tray icon). The library is static, one folder
per layer, each a library the next one links, so the tests and every part
use the same code; the command line and the app don't know each other.

**Nothing blocks.** Every engine operation is a job, an
`EngineJob` (`lib/core/enginejob.h`): a `KJob` that runs without waiting and
says when it's done with its `result()` signal, so the app can use the
engine without freezing. On failure, its `errorText()` says why,
translated. Its `output(line)` signal gives what the engine prints as it
goes (Podman's standard error: an image's download, its errors), for
whoever wants to show it, or, with `setOutputToTerminal(true)`, goes
straight to the process's standard error, as the engine writes it on its
own. The command line runs each job with `exec()`, which waits in an event
loop: on a terminal, with `setOutputToTerminal(true)`, so Podman draws its
progress bars; elsewhere (a pipe, a file), printing its output to stderr
line by line.
`enter`'s job (`EnterJob`) runs the shell on the process's terminal, which
podman reads and writes itself, and ends when the shell exits, with its
exit status: the shell failing isn't the job failing. For a terminal that
isn't the process's (the app's), `Engine::shellCommandLine()` gives the
command that opens the shell, for the terminal to run.

**Following changes.** `Engine::watch()`'s job (`WatchJob`) runs until it's
killed and emits `changed()` when a workbox may have changed, wherever it
changed (workbox, a terminal, another program), without polling. It doesn't
say what changed: whoever follows lists again. Several changes in a row (one
operation makes a few) give one `changed()`, 100 ms after the last. Start it
before the first `list()`, so no change between them is missed.

| Folder | Library | What it is |
| --- | --- | --- |
| `lib/core/` | `workbox-core` | What no engine knows. `workbox.h`: the model, a workbox (id, name, color, and what only the engine knows: whether it's running, its image), its labels and its volume's name. `color.h`: `--color`'s formats, the palette, the pick and the suggestions after it. `settings.h`: `workbox.conf`. `enginejob.h`: an engine's job. `engine.h`: what workbox needs from an engine (list, watch, create, start, stop, enter, remove, each a job). `terminalcolors.h`: the terminal's colors for a workbox, what sets them, and the prompt. `volumes.h`: `-v` both ways, the folders that can be shared, and how a volume is shown. `commands.h`: what the commands do on any engine: `CreateJob` (lists, checks the id, picks the color, creates), `find` and `sortedById` |
| `lib/engines/podman/` | `workbox-podman` | `PodmanEngine`, the only code that knows Podman's commands, and `PodmanJob`, which runs them one after another with `QProcess`'s signals (with cleanup commands to undo the earlier ones when one fails, never when the first one does), `PodmanEnterJob`, `podman exec`'s shell on the terminal, and `PodmanWatchJob`, `podman events`. Another engine would go in `lib/engines/<name>/` |
| `app/` | `workbox-model`, `workbox-window` | `workboxmodel.h`: `WorkboxModel`, the list of the window and the tray icon on the library. `window/`: the window (`mainwindow.h`), its rows and swatch (`workboxdelegate.h`), its dialogs (`dialogs/`: creating a workbox, `dialogs/workbox/create.h`, adding a volume to it, `dialogs/volume/add.h`, and the parts of their forms, `dialogs/formparts.h`), its terminals, Konsole's part (`terminals.h`), and the tray icon (`trayicon.h`), out of the program (`main.cpp`) so the tests link them. `icons/`: the app's icon and the toolbox. `com.brescaos.workbox.desktop.in`: the app's `.desktop` |
| `cli/` | (the program) | `main.cpp`, the command line: arguments, help, messages, `--version`. `completion.bash`, its bash completion: keep it in sync with the commands and options; the ids come from `workbox list --quiet` |

- A workbox is a container with the labels `com.brescaos.workbox.name`,
  `com.brescaos.workbox.color` and, with `--sudo`,
  `com.brescaos.workbox.sudo=true`, the only copy of its settings; the
  name's label is how workbox finds its containers.
- `podman create --userns=keep-id --network=pasta` with its labels,
  `--volume workbox-<id>.home:$HOME`, and per volume `--volume
  <folder>:<target>:z[,ro]`, `--volume workbox-<id>.<name>:<target>[:ro]`
  or `--volume <target>[:ro]`, running `sleep infinity`: the same uid inside, so files
  in shared folders keep their owner, its own network (outgoing only),
  nothing else of the host. The volumes are a label too,
  `com.brescaos.workbox.volumes` (JSON).
- With `--sudo`, `create` starts it, its root writes
  `#<uid> ALL=(ALL) NOPASSWD: ALL` to `/etc/sudoers.d/workbox` (mode 0440)
  and stops it. A file mounted from the host wouldn't be owned by the
  workbox's root, and sudo would refuse it. If that fails, it removes the
  workbox: nothing is left half done.
- `start` is `podman start`; `stop` is `podman stop --time 0`, and `rm` is
  `podman rm --force --time 0 --volumes` (its anonymous volumes too) then
  `podman volume rm` of its home and its named volumes (sleep ignores
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
  `--workdir /root`. With `--wayland`, `create` adds `--volume
  <socket>:/run/workbox/wayland-0:z`, `--mount
  type=tmpfs,destination=/run/user/<uid>,tmpfs-mode=0700,U=true`,
  `--device /dev/dri` and their `--env`s. With a command, `<id> <command>…` in place of
  `bash --login`, and no `PS1` or `WORKBOX_NAME`.
- Podman's standard error is the job's output, so on the command line it
  shows as it did; the job's error says which command failed.

## Translations

`i18n()`, `i18nc()` and `i18np()` in the C++ and
`app/com.brescaos.workbox.desktop.in`, all in the domain `workbox`:
`po/<lang>.po`, for now with the rest of the repository's
([`docs/translations.md`](../../../docs/translations.md)). The commands and
options aren't translated; Qt's own words in the help (`Usage:`,
`Options:`) come from Qt's translations.

## Tests

- With `-DBUILD_TESTING=ON`, `ctest` runs its tests (`make check-workbox`
  in this repository), each in `tests/` at its source's path
  (`tests/lib/core/colortest.cpp` tests `lib/core/color.cpp`): the model, the colors and the pick, the terminal's
  colors and their contrast, the settings, which folders can be shared
  (`tests/lib/core/sharestest.cpp`), the commands on a fake engine,
  `PodmanEngine`'s arguments with the fake `podman` of `tests/fakes/`, and
  the app's list on it (`tests/app/`): its rows, following events,
  each operation, its failure and the problem without settings; and its
  window, offscreen (`tests/app/window/`): the list, its search, start,
  stop, remove only once the id is typed, opening a terminal from the tray,
  no workboxes, the problem, and the command line that sets the terminal's
  colors; its create dialog: the id from the name, a taken id, the color, from
  the suggestions and the color dialog, its volumes
  through their dialog, edited and removed; the volume dialog: its types,
  the folder in the workbox filled in, a blocked folder, editing one; what it creates with and its row at once, and a failure;
  and its tray icon: how many run, its icon with and without the dot, and its menu. Konsole's
  part and Plasma's tray themselves aren't tested.
  They also check that nothing blocks: results come after `start()`
  returns, and the event loop keeps running while podman does; a job's
  output, and killing it; that several events are one change, and that
  `podman events` is started again when it ends.
- `tests/cli/workbox.bats` checks an installed workbox, for now in this
  repository's image (`tests/Containerfile`, `make test`): its own settings,
  that `create` takes the image the user's settings name over the others,
  `-v`'s mounts and refusals, `info`,
  `--version`, its translations, its bash completion, the app, its
  `.desktop`, its icons and Konsole's part installed, with the fake `podman`.

Neither runs the real Podman.
