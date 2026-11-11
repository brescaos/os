# Bresca Welcome

The welcome BrescaOS shows at a user's first Plasma login, as a short wizard:
a welcome page (what BrescaOS is, the alpha and Fedora notices), the setup
steps and a last page. Today there are no steps: only the welcome page and
the last one.

| Source | In the image |
| --- | --- |
| `main.cpp`, `revision.cpp`, `qml/` | `/usr/bin/bresca-welcome`, with the QML compiled in |
| `com.brescaos.welcome.desktop.in`, translated | `/usr/share/applications/com.brescaos.welcome.desktop` (application menu) |
| `com.brescaos.welcome-autostart.desktop.in`, translated | `/etc/xdg/autostart/com.brescaos.welcome-autostart.desktop` (`bresca-welcome --login` at every Plasma login) |
| `po/<lang>.po` | `/usr/share/locale/<lang>/LC_MESSAGES/bresca-welcome.mo` |

It's C++ with a Qt Quick / Kirigami interface. `image/Containerfile` compiles
it in its own stage, `welcome-build`, so the compilers and `-devel` packages
stay out of the image. It runs as the user: no root, no polkit, no network.

## When it opens

`Revision::current` in `revision.h` is the welcome's revision. Each user's
`~/.local/state/bresca-welcomerc` keeps the one they were last shown:

```ini
[General]
ShownRevision=1
```

| `bresca-welcome --login` (autostart) | |
| --- | --- |
| No state (first login), or `ShownRevision` lower than `Revision::current` | Opens with every page, and records `Revision::current` |
| `ShownRevision` the same or higher | Exits without a window |

- It records the revision as the window opens, so it opens once even if the
  user skips it or closes it.
- It always shows every page, whatever the user saw before.
- From the application menu ("Welcome to BrescaOS", `bresca-welcome`) it
  shows the same pages and records nothing.
- Higher, not just different, so going back to a previous BrescaOS version
  (a lower revision) doesn't open it, nor does coming back to the newer one.

To have it open again after an update, a BrescaOS version raises
`Revision::current` by one. Never lower it.

To see the first login again, in a terminal:
`rm ~/.local/state/bresca-welcomerc; bresca-welcome --login`.

## Steps

Each step is a page that knows nothing of the others: the window only gets
their list. They apply what the user picks at once; there is no Apply at the
end.

### Adding a step

1. Write `qml/steps/<id>/<Name>Step.qml` with `StepPage` as its root item
   (`title`, `text`, then its own controls). If it needs C++ to read or
   change the system, that goes in the same folder too, as a QML singleton of
   the module (`QML_ELEMENT`, `QML_SINGLETON`), in `SOURCES`.
2. Add it to `QML_FILES` in `CMakeLists.txt`.
3. Add `Qt.resolvedUrl("steps/<id>/<Name>Step.qml")` to `steps` in
   `qml/Main.qml`, where it should appear.
4. Wrap its texts in `KI18n.i18n()` (`import org.kde.ki18n`), run `make pot`
   and translate the new strings ([translations](../../../docs/translations.md)).
5. Add `tests/qml/tst_<id>.qml` for what it does: every `tst_*.qml` runs in
   `qmltest`.
6. If every user should see it after the update that brings it, raise
   `Revision::current`.

### Fedora notice

The welcome page says that BrescaOS is not provided or supported by the
Fedora Project, and links to <https://fedoraproject.org/>. Fedora's
trademark guidelines require that notice, shown prominently, from systems
that combine Fedora with other software (BrescaOS's README, "Fedora
trademarks"). Keep it on the welcome page.

## Layout

| Path | What |
| --- | --- |
| `main.cpp` | Entry point, `--login` |
| `revision.h`, `revision.cpp` | `Revision::current` and the per-user state |
| `qml/Main.qml` | The window: the list of steps, the pages and Skip / Back / Next |
| `qml/StepPage.qml` | Layout every page uses |
| `qml/WelcomePage.qml`, `qml/DonePage.qml` | The first and the last page |
| `qml/steps/<id>/` | The steps (none yet) |

The welcome page's title has the BrescaOS version: `VERSION_ID` in
`/usr/lib/os-release` (`QSysInfo::productVersion()`), which
`image/build/identity.sh` writes. Its icon is the icon theme's `help-about`
until the BrescaOS logo is in the image.

## Translations

Domain `bresca-welcome`: `i18n()` in `main.cpp`, `KI18n.i18n()` in the QML,
and the English-only `.desktop.in` templates, which
`image/build/compile_translations.sh` fills in with `Name[<lang>]=` and the
like.

## Tests

Everything but `main()` is a static library, `welcome-core` (the revision
state and the QML module), so the tests use the program's own code.

```bash
make check-welcome   # (in os/) compiles it with its tests into build/welcome/ and runs them
```

Part of `make check`. It needs `cmake extra-cmake-modules gcc-c++
qt6-qtdeclarative-devel kf6-ki18n-devel kf6-kconfig-devel`, plus
`kf6-kirigami kf6-qqc2-desktop-style` to load the QML. Offscreen: no display
needed.

| Test | What it covers |
| --- | --- |
| `logintest` | The real program with its own `XDG_STATE_HOME`: the first login opens and records the revision, an older one opens and records it, the same or a newer one (rollback) exits without a window, the menu opens and records nothing |
| `qmltest` (`tests/qml/tst_*.qml`) | Without steps: the welcome page (its title has the version) and the last one. With `tests/qml/SampleStep.qml`, a step only the tests have: the steps go in between, in order, and Skip / Back / Next walk through them. No page scrolls at the default size; the Fedora notice is at the bottom of the welcome page |

`tests/Containerfile` (`make test`, on the host) checks in the BrescaOS
image that the program, its `.desktop` files and its translations are
installed, that `--version` gives the image's version, and that it opens at
a first login and not again.

## Development

In the development VM hot reload compiles it again when its sources change,
and its `.desktop` files when its translations do; close it and open it
again to see the change. `.clangd` gives clangd the include folders without a
build.

```bash
bresca-welcome                                                 # as from the menu
rm -f ~/.local/state/bresca-welcomerc; bresca-welcome --login  # as at a first login
LANGUAGE=es bresca-welcome                                     # in Spanish (or ca)
```
