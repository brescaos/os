# Distrobox

BrescaOS's integration of Distrobox: each distrobox's colors in the terminal,
and a system tray applet to start, stop and remove them.

## Files

| File | Installed as | What it does |
|---|---|---|
| `distrobox.conf` | `/usr/share/defaults/distrobox/distrobox.conf` | In `distrobox create`, mounts `profile.sh`; in `distrobox enter`, runs `bresca-distrobox-colors` |
| `terminalcolors.cpp`, `main.cpp` | `/usr/libexec/bresca/distrobox/bresca-distrobox-colors` | Prints the colors of the distrobox `distrobox enter` enters |
| `profile.sh` | `/usr/libexec/bresca/distrobox/profile.sh`, in every distrobox as `/etc/profile.d/zz-brescaos-distrobox.sh` (read-only) | Background, prompt and Konsole's container indicator |
| `distroboxmodel.cpp`, `distroboxmodelqml.h` | `/usr/lib64/qt6/qml/com/brescaos/distrobox/` | The tray applet's list and actions, the QML module `com.brescaos.distrobox` |
| `applet/` | `/usr/share/plasma/plasmoids/com.brescaos.distrobox/` | The tray applet (`metadata.json` from `metadata.json.in`, with the translations) |
| `applet/package/contents/icons/distrobox-symbolic.svg` | also `/usr/share/icons/hicolor/scalable/apps/com.brescaos.distrobox.svg` | The applet's icon in its metadata (About, the tray's settings), which takes only a theme icon's name |
| `po/` | `/usr/share/locale/<lang>/LC_MESSAGES/plasma_applet_com.brescaos.distrobox.mo` | The applet's translations ([docs/translations.md](../../../docs/translations.md)) |

## Terminal colors

Entering a distrobox in a terminal turns the background a dark color of its
own, and the prompt shows its name in a light shade of that color:

```
📦 dev [user@bresca ~]$
```

The color has the hue Konsole gives that distrobox, so in Konsole it matches
the tab and the dot of the footer label ("Distrobox: dev"). Leaving the
distrobox gives the terminal its own colors back. Nothing is stored: the
color follows from the name each time.

### How they work

**Distrobox's configuration.** Every Distrobox command sources its
`distrobox.conf` files with `/bin/sh`, with the command's arguments in `"$@"`,
in order: `/usr/share/distrobox/` (the default image, written by
`image/build/system.sh`), then `/usr/share/defaults/distrobox/` (this one),
then `/etc/distrobox/` and the user's, which override it.

- `distrobox-create` (also behind `assemble`, `ephemeral` and `create --clone`)
  mounts `profile.sh` into the new distrobox.
- `distrobox-enter` gets `bresca-distrobox-colors "$@"`'s output as extra
  `podman exec` flags: `--env=BRESCAOS_DISTROBOX_COLOR=#…` (background) and
  `--env=BRESCAOS_DISTROBOX_LABEL_COLOR=#…` (prompt label), one per line, as
  distrobox-enter takes them.

**The colors (`bresca-distrobox-colors`).** It runs on the host, as the user,
in about 10 ms.

1. The name: it reads `distrobox enter`'s arguments as distrobox-enter does
   (`--name`/`-n`, or the argument that isn't a flag; `-a` takes a value;
   `-e`, `--exec` and `--` start the command), else `$DBX_CONTAINER_NAME`,
   else Distrobox's default, `my-distrobox`. A `container_name_default` set in
   another `distrobox.conf` isn't seen.
2. Konsole's color for it, as Konsole 26.08 computes it
   (`ContainerSessionState::colorForContainerKey`): `qHash` of
   `distrobox:<name>`, truncated to 32 bits, gives the HSV hue
   (`hash % 360`), saturation (130–219) and value (170–229); then 3/4 of
   that and 1/4 of the accent, Konsole's palette highlight, which Plasma
   takes from the color scheme's selection background (`KColorScheme`, in
   `kdeglobals`). Linking the same Qt as Konsole gives the same `qHash`.
3. That color's OKLCH hue gives the background, at lightness 0.28 and chroma
   0.048 (the most sRGB has at that lightness for every hue, so all
   backgrounds look alike but for the hue), and the label, at lightness 0.84
   and up to 0.11 chroma, less where sRGB doesn't have it.

For every hue, text `#fcfcfc` on the background and the label on it have a
contrast of at least 7:1 (WCAG AAA); the tests check it every 0.5°.

**Entering.** `distrobox enter` starts a login shell, which sources
`/etc/profile.d/`. `zz-` puts `profile.sh` after Distrobox's
`distrobox_profile.sh` and the image's own files (such as Toolbox's
`toolbox.sh`), so its prompt wins; a `PS1` set in `~/.bashrc` still wins over
it. Only an interactive bash entered with colors changes anything:

- background with OSC 11 and text (`#fcfcfc`) with OSC 10, standard xterm
  sequences, so any terminal that has them follows;
- in Konsole (`KONSOLE_DBUS_SESSION` is set), OSC 777
  `container;push;<name>;distrobox`. Konsole's own detection looks at the
  processes only once, when `distrobox enter` starts, often before the
  container has started, and then never shows its footer label or colors
  the tab. Told this way, it does both every time, with its color for the
  name;
- the prompt: `📦 name`, bold in the label's color, then Fedora's
  `[\u@\h \W]\$ `.

On exit (an `EXIT` trap), OSC 111 and 110 put back the terminal profile's
background and text (Konsole ignores them without an argument, so they're
sent with an empty one), and in Konsole OSC 777 `container;pop;;` says the
shell left the distrobox.

### Limits

- Distroboxes created before this file existed don't have `profile.sh`, so
  they keep Distrobox's prompt and the terminal's colors.
- Only an interactive bash: `distrobox enter name -- command`, zsh and fish
  don't change.
- The color depends on the name only, so two distroboxes can get close hues;
  the accent's quarter pulls them towards it.
- It follows Konsole's color for the system's color scheme. A different
  window color scheme chosen in Konsole itself, a Konsole that computes its
  color differently, or a Qt whose `qHash` changes would set them apart.
- If the shell is killed (`podman stop` from another terminal), the trap
  doesn't run and the colors stay until the tab is closed.
- In the development VM, a running distrobox keeps the `profile.sh` it
  started with: the hot reload writes a new file. `distrobox stop` it.

## Tray applet

The system tray always shows Distrobox's symbol (a cube with a prompt,
`applet/package/contents/icons/distrobox-symbolic.svg`, drawn for BrescaOS),
monochrome like the tray's other symbols, with the number of running
distroboxes in a badge on its corner ("9+" from ten on), none while none runs.
Its tooltip says how many run.

Clicking it lists every distrobox of the user, running or not, by name. Each
row has the symbol in Konsole's color for that distrobox (the tab's and the
footer label's dot, so the background's hue too) and its name, both at 70 %
opacity while it's stopped, and two buttons:

- **Start** or **Stop.** Start runs `distrobox enter <name> -- true`, which
  returns once the first start's setup is done, so its spinner lasts as long.
  Stop runs `distrobox stop --yes <name>`; when that fails, `podman inspect`
  decides, since with a terminal open in it podman stops it and then fails
  ("has active exec sessions, refusing to clean up"): stopped is stopped.
- **Remove,** which asks in the row itself, not in a dialog, so the list stays
  open: the warning that everything installed in it is lost, with Remove and
  Cancel. Remove runs `distrobox rm --force <name>`.

While one runs, a spinner takes its button's place and the row's other button
is disabled, so a distrobox has one operation at a time; others don't wait.
Once it's done, the row shows the new state at once (removed, the row goes).
A failure stays in the row, with what the command said, until its next
operation. With no distroboxes, the list says how to create one.

**The list (`DistroboxModel`).** It lists the distroboxes with
`podman ps --all --filter label=manager=distrobox --format json` when it
starts, and again after every event `podman events` reports for a distrobox
container (several in a row give one listing, 100 ms later), so it follows
what changes them anywhere: the applet, a terminal, another program. No
polling; if `podman events` ends, it starts again 10 seconds later. The rows
change one by one, so a row asking to remove keeps asking while others change.
The color is `TerminalColors::konsoleTabColor()` (above), with the palette's
highlight as the accent: the applet runs in plasmashell, which gets the same
palette from Plasma as Konsole.

It's a Plasma applet, `com.brescaos.distrobox`, in the system tray because its
metadata says it's a notification area applet (`X-Plasma-NotificationArea`),
enabled by default, so the tray loads it for every user. Its texts are in its
QML, with `i18n()`: Plasma gives it the domain
`plasma_applet_com.brescaos.distrobox`. The C++ has none: from a failure it
passes on what the command said.

## Build and tests

Built in `image/Containerfile`'s `distrobox-build` stage (cmake,
extra-cmake-modules, gcc-c++, qt6-qtbase-devel, qt6-qtdeclarative-devel,
kf6-kcolorscheme-devel, kf6-kconfig-devel); the image already has the
libraries they use, for Plasma and Konsole. The applet's QML is copied as it
is.

- `make check-distrobox` (part of `make check`): compiles it with its tests
  into `build/distrobox/` and runs them: `tests/terminalcolorstest.cpp` (the
  name, Konsole's color, the hues and the contrast) and
  `tests/distroboxmodeltest.cpp` (the list, the events, the three operations,
  one at a time, and their failures, with the fake `podman` and `distrobox`
  of `tests/fakes/`).
- `tests/` (`make test`, on the host): bats in the BrescaOS image, for
  `distrobox.conf`, `profile.sh` and the applet's files.
- The applet's QML needs Plasma, which `make check` doesn't have: the
  development VM tries it.
