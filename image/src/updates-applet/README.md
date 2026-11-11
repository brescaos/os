# BrescaOS Updates applet

A Plasma applet in the system tray that tells when a new BrescaOS version is
available, and downloads and installs it from its popup.

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
- The applet only reads that file when it changes: `inotifywait`
  (inotify-tools) sleeps in the kernel until `check.json` is moved into
  place or removed, then the applet reads it and waits again.
- Its icon is Breeze's update status icon, as Plasma's own update notifier:
  `update-none` when it's up to date (hidden among the tray's idle items),
  `update-medium` with a version available (`update_available`: shown), and
  `system-reboot` with a version installed that waits for a restart
  (`install.json`'s `installed_version`: shown, with **Restart Now** in the
  popup, until the restart empties `/run/bresca/updates/`). The popup's big
  icon is the same. A successful install removes `check.json`, so the
  available version gives way to the one waiting for the restart.
- Its popup, as tall as what it shows, has one row of buttons for the state:
  - Up to date: **Check for Updates**, which starts `bresca-updates@check`
    (`systemctl start --no-block`).
  - A version available: **Download** and **Download & Install**
    (`bresca-updates@download`, `@download-install`). polkit asks for an
    administrator's password; cancelling it changes nothing. From a registry
    allowed without TLS, or when it can't tell, it asks first in the popup,
    with the CLI's warning, as the CLI's `--no-tls` does: the services don't
    check it. It knows from `status.json`: it starts `bresca-updates@status`
    as it starts, without a password dialog (`--no-ask-password`).
  - Downloading or installing: a progress bar with the download's phase and
    percent (`download-progress.json`), as System Settings shows it. Only
    then it asks systemd and reads the progress every second; one started
    elsewhere (System Settings, the CLI) shows too, once it removes its last
    result.
  - A version downloaded: **Install** (`bresca-updates@install`).
  - A version installed: **Restart Now**.
  - A failed download or install: bootc's error, above the buttons.
- Its configure action (the popup header's button and the context menu's
  **Configure BrescaOS Updates…**) opens System Settings > BrescaOS Updates
  too (`KCMLauncher`).
- The notification, event `updateAvailable` of `brescaos-updates.notifyrc.in`,
  comes when the applet sees a version for the first time in a session: when
  a check finds it, and at every login while it isn't installed, as a
  reminder. Its **Install** opens System Settings > BrescaOS Updates. It
  stays until closed. Settings > Notifications > Applications > BrescaOS
  Updates configures or mutes it.
- Two more notifications, from the results of a download and an install,
  started from System Settings > BrescaOS Updates or the CLI. They're watched
  as `check.json` is, and notified only when they change while the applet
  runs, never one from before it started:
  - `updateDownloaded`: a version downloaded without installing it
    (`download.json` with `download_only`; Download and Install's isn't).
    Its **Install** installs it, as the popup's does. It says that
    restarting first discards the download: bootc doesn't keep a version
    only downloaded across a reboot.
  - `updateInstalled`: a version installed (`install.json` with
    `installed_version`). Its **Restart Now** opens Plasma's restart prompt
    (`org.kde.LogoutPrompt`, with `gdbus`).

The system tray loads it on its own: `X-Plasma-NotificationArea` and
`EnabledByDefault` in `metadata.json.in`. In the development VM hot reload copies
a change and restarts plasmashell, which loads it.

`tests/` (`make test`) checks, in the BrescaOS image, that its files, its
translations (and the translated notifyrc and metadata.json) and
`inotifywait` are installed. Its QML only runs in Plasma: try it in the
development VM.
