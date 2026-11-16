# BrescaOS Updates applet

A Plasma applet in the system tray that tells when a new BrescaOS version is
available, downloaded or installed.

| Source | In the image |
| --- | --- |
| `package/` (`contents/ui/main.qml`) | `/usr/share/plasma/plasmoids/com.brescaos.updates/` |
| `metadata.json.in`, translated | `/usr/share/plasma/plasmoids/com.brescaos.updates/metadata.json` |
| `brescaos-updates.notifyrc.in`, translated | `/usr/share/knotifications6/brescaos-updates.notifyrc` |
| `po/<lang>.po` | `/usr/share/locale/<lang>/LC_MESSAGES/plasma_applet_com.brescaos.updates.mo` |

Its version is in `VERSION.txt`: the build writes it into `metadata.json`.

Its texts are translated ([`docs/translations.md`](../../../docs/translations.md)):
`i18n()` in the QML, and the English-only templates `metadata.json.in` and
`brescaos-updates.notifyrc.in`, which the build fills in with the
translations of `po/`.

## How it works

- `bresca-updates-check.timer` runs `bresca-updates@check` 5 minutes after
  boot and then every 6 hours (`image/src/bresca-cli/`). It only checks. Its
  result is `/run/bresca/updates/check.json`, readable without root.
- The applet only reads the results (`check.json`, `download.json` and
  `install.json`) when they change: `inotifywait` (inotify-tools) sleeps in
  the kernel until one is moved into place or removed. The applet reads them
  half a second after each wait starts, so a change that came between two
  waits isn't missed, and waits again. It never asks systemd and starts no
  operation.
- It's in the tray only with something to do, and not at all otherwise
  (`HiddenStatus`): a version available (`check.json`'s `update_available`),
  downloaded without installing it (`download.json` with `download_only`),
  or installed and waiting for a restart (`install.json`'s
  `installed_version`, until the restart empties `/run/bresca/updates/`). A
  successful install removes `check.json`, so the available version gives
  way to the one waiting for the restart; while it downloads or installs,
  it's still the available one.
- Its icon is BrescaOS Updates' symbol (`package/contents/icons/`), in the
  panel's text color, with the seal of the System Settings module's state
  icons at its top right, in the logo's amber; with a version waiting for a
  restart, a restart arrow in place of the download one. Its tooltip says
  which version and what it waits for.
- A click opens System Settings > BrescaOS Updates (`KCMLauncher`), and so
  does its context menu's **Configure BrescaOS Updates…**.
- The notification, event `updateAvailable` of `brescaos-updates.notifyrc.in`,
  comes when the applet sees a version for the first time in a session: when
  a check finds it, and at every login while it isn't installed, as a
  reminder. Its **Install** opens System Settings > BrescaOS Updates. It
  stays until closed. Settings > Notifications > Applications > BrescaOS
  Updates configures or mutes it.
- Two more notifications, from the results of a download and an install,
  started from System Settings > BrescaOS Updates or the CLI. They're
  notified only when they change while the applet runs, never one from
  before it started:
  - `updateDownloaded`: a version downloaded without installing it
    (`download.json` with `download_only`; Download and Install's isn't).
    Its **Install** opens System Settings > BrescaOS Updates, which installs
    it (the module's `install` argument). It says that restarting first
    discards the download: bootc doesn't keep a version only downloaded
    across a reboot.
  - `updateInstalled`: a version installed (`install.json` with
    `installed_version`). Its **Restart Now** opens Plasma's restart prompt
    (`org.kde.LogoutPrompt`, with `gdbus`).

  Each closes the ones it leaves behind: `updateDownloaded` the version
  available, `updateInstalled` that one and the version downloaded.

The system tray loads it on its own: `X-Plasma-NotificationArea` and
`EnabledByDefault` in `metadata.json.in`. In the development VM hot reload copies
a change and restarts plasmashell, which loads it.

`tests/` (`make test`) checks, in the BrescaOS image, that its files, its
translations (and the translated notifyrc and metadata.json) and
`inotifywait` are installed. Its QML only runs in Plasma: try it in the
development VM.
