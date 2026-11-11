# Distrobox colors

Entering a distrobox in a terminal turns the background a dark color of its
own, and the prompt shows its name in a light shade of that color:

```
📦 dev [user@bresca ~]$
```

The color has the hue Konsole gives that distrobox, so in Konsole it matches
the tab and the dot of the footer label ("Distrobox: dev"). Leaving the
distrobox gives the terminal its own colors back. Nothing is stored: the
color follows from the name each time.

## Files

| File | Installed as | What it does |
|---|---|---|
| `distrobox.conf` | `/usr/share/defaults/distrobox/distrobox.conf` | In `distrobox create`, mounts `profile.sh`; in `distrobox enter`, runs `bresca-distrobox-colors` |
| `terminalcolors.cpp`, `main.cpp` | `/usr/libexec/bresca/distrobox/bresca-distrobox-colors` | Prints the colors of the distrobox `distrobox enter` enters |
| `profile.sh` | `/usr/libexec/bresca/distrobox/profile.sh`, in every distrobox as `/etc/profile.d/zz-brescaos-distrobox.sh` (read-only) | Background, prompt and Konsole's container indicator |

## How it works

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

## Limits

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

## Build and tests

Built in `image/Containerfile`'s `distrobox-build` stage (cmake, gcc-c++,
qt6-qtbase-devel, kf6-kcolorscheme-devel, kf6-kconfig-devel); the image already
has the libraries it uses, for Plasma and Konsole.

- `make check-distrobox` (part of `make check`): compiles it with
  `tests/terminalcolorstest.cpp` (the name, Konsole's color, the hues and the
  contrast) into `build/distrobox/` and runs it.
- `tests/` (`make test`, on the host): bats in the BrescaOS image, for
  `distrobox.conf` and `profile.sh`.
