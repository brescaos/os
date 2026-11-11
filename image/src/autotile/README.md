# Autotiling

A KWin script that places windows on their own so they fill the screen, with
its System Settings page and its system tray applet. Off by default: turned
off, the script is unloaded and KWin works as usual, windows staying where
they are.

| Source | In the image |
| --- | --- |
| `script/layouts/*.js`, then `script/main.js`, in one file | `/usr/share/kwin/scripts/com.brescaos.autotile/contents/code/main.js` |
| `script/metadata.json.in`, translated | `/usr/share/kwin/scripts/com.brescaos.autotile/metadata.json` |
| `common/`, `qml/` | `/usr/lib64/qt6/qml/com/brescaos/autotile/`: the QML module `com.brescaos.autotile` |
| `kcm.cpp`, `ui/` and `kcm_brescaos_autotile.json.in`, translated | `/usr/lib64/qt6/plugins/plasma/kcms/systemsettings/kcm_brescaos_autotile.so` |
| `kcm_brescaos_autotile.json.in`, translated | `/usr/share/applications/kcm_brescaos_autotile.desktop` |
| `applet/package/` | `/usr/share/plasma/plasmoids/com.brescaos.autotile/` |
| `applet/metadata.json.in`, translated | `/usr/share/plasma/plasmoids/com.brescaos.autotile/metadata.json` |
| `po/<lang>.po` | `/usr/share/locale/<lang>/LC_MESSAGES/kcm_brescaos_autotile.mo` |

`image/Containerfile` compiles the script, the QML module and the KCM in its
own stage, `autotile-build`, so the compilers and `-devel` packages stay out
of the image; it copies the applet as it is. It all runs as the user: no
root, no polkit, no network. It only writes the user's `~/.config/kwinrc`
and talks to KWin over the session bus.

## What the user sees

- **System Settings > Window Management > Automatic Tiling**
  (`kcm_brescaos_autotile`): a switch for autotiling and, while it's on, the
  layouts (a card each: a radio button, its symbol, name and description)
  and the gap between windows (8 px by default, up to 64, none at the
  screen's edges). A checkbox shows the tray icon. The changes wait for
  Apply; Defaults is off, Binary tree, 8 px, no icon.
- **The tray applet**: hidden unless that checkbox is on (`ShowInTray`). Its
  icon is the current layout's monochrome symbol, or two free windows while
  it's off. Its popup has the switch and, while it's on, the layouts, which
  apply at once, and a link to the KCM.
- The checkbox of **Autotiling** in System Settings > KWin Scripts is the same
  switch.
- **Meta+Shift+F** takes the active window out of the layout and puts it back.
  Its name in System Settings > Shortcuts is English only: KWin shows no
  `i18n` for plain JavaScript scripts.

## Layouts

The layout applies to every desktop and screen at once; each virtual desktop
and screen has its own arrangement.

- **Binary tree** (the default): each new window splits the last focused one
  along its longer side; when a window closes, its sibling takes the space.
- **Master**: one main window on the left and the others stacked on the
  right, all of a height; a new one goes to the bottom, and resizing moves
  the line between the two sides.
- **Grid**: all windows the same size, in rows as square as they can be, a
  shorter last row sharing the width; a new one goes last, and resizing puts
  it back.

When every window is placed at once (the script loading, a change of
layout), the active one takes the biggest place and the others follow by
most recent use.

## How it works

- **State, all of it in `~/.config/kwinrc`.** The script is on when
  `[Plugins] com.brescaos.autotileEnabled` is; `Layout`, `Gap` and
  `ShowInTray` are in the script's group, `[Script-com.brescaos.autotile]`.
  `AutotileLayout` (`common/`) reads and writes them for the KCM and the
  applet. It watches the file (`KDirWatch`) rather than KConfig's change
  notifications, since System Settings > KWin Scripts writes it without
  them.
- **Turning it on and off** saves the switch and calls KWin's
  `org.kde.kwin.Scripting.start`, which loads what's on and unloads what
  isn't. Turning it off has KWin reconfigure too: unloaded, the script can't
  undo what it changed in KWin's running options (below).
- **Layout and gap**: saved, then KWin reconfigures, which is when the script
  reads its settings again (`readConfig`). A layout no longer known is the
  default, in the script and in `AutotileLayout`.
- **Windows that stay out**: dialogs, fixed-size, minimized, maximized and
  full screen windows, those on all desktops, those hidden from both the task
  bar and Alt+Tab (such as `xwaylandvideobridge`, which is invisible), and
  Bresca Welcome.
- **New windows**: KWin only tells scripts about a window once it's drawn, at
  the size the app chose, so the window stays hidden until it has drawn
  itself at its place (300 ms at most).
- **Minimum sizes**: a window can't get smaller than its minimum, which may
  be more than its place has. While it's in use (its dialogs count), its
  place grows to fit it and the others shrink evenly, with no overlap; out
  of use they all go back. A neighbour that reaches its own minimum sticks
  out of its place.
- **Dragging**: resizing a window moves the split. Dropping a moved window on
  another swaps the two; dropped anywhere else, it goes back to its place.
- **Edge snapping**: while the script is on, dropping a window on a screen
  edge doesn't maximize it or give it half or a quarter of the screen. It
  only changes KWin's running options (`electricBorderMaximize`,
  `electricBorderTiling`), never the user's settings, and KWin reads them
  back when it's turned off.
- **One file**: KWin loads one file and scripts can't include others, so the
  build puts `script/layouts/*.js` and then `script/main.js` in
  `contents/code/main.js`. Layouts know nothing of KWin, only ids and
  rectangles: a new one is a file in `script/layouts/` with the methods
  listed at the top of `script/main.js`, one more entry in its `LAYOUTS`, its
  row in `AutotileLayout::layouts()` and its symbol in `qml/icons/`.

Known limit: activities aren't told apart.

## Translations

One domain for the whole component, `kcm_brescaos_autotile` (the KCM's
plugin id; [`docs/translations.md`](../../../docs/translations.md)):
`i18n()` in `common/autotilelayout.cpp` (the layouts' names) and in the KCM's
QML, `i18nd("kcm_brescaos_autotile", …)` in the applet's. The three
metadata files are English-only `*.in` templates that
`image/build/compile_translations.sh` fills in; the KCM's is compiled into
the module (`TRANSLATED_METADATA_DIR`), as in `updates-kcm`.

## Development

`make check-autotile` (part of `make check`) compiles it with its tests
into `build/autotile/` and runs them:

- `tests/qml/tst_script.qml` runs the script, as the build puts it
  together, in Qt's JavaScript engine, the one KWin uses, on a stub of KWin's
  API (`tests/qml/kwinstub.js`): the layouts on their own, the script driving
  a fake workspace, and that the script and `AutotileLayout` list the same
  layouts and defaults.
- `tests/autotilelayouttest.cpp` checks `AutotileLayout` on a temporary
  `kwinrc`.

`tests/` (`make test`) also checks, in the BrescaOS image, that the script,
the applet, the KCM and the QML module are installed and translated. How it
all behaves only runs in KWin and Plasma: try it in the development VM, whose
hot reload compiles it again, reloads the script in KWin and restarts
plasmashell when the applet or the QML module change.

The build dependencies are `cmake`, `extra-cmake-modules`, `gcc-c++`,
`kf6-kcmutils-devel`, `kf6-kconfig-devel`, `kf6-kcoreaddons-devel`,
`kf6-ki18n-devel` and `qt6-qtdeclarative-devel`. `.clangd` gives clangd the
include folders without a build.
