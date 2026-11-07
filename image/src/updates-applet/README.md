# BrescaOS Updates applet

A Plasma applet in the system tray that tells when a new BrescaOS version is
available.

| Source | In the image |
| --- | --- |
| `package/` (`metadata.json`, `contents/ui/main.qml`) | `/usr/share/plasma/plasmoids/com.brescaos.updates/` |
| `brescaos-updates.notifyrc` | `/usr/share/knotifications6/brescaos-updates.notifyrc` |

## How it works

- `bresca-updates-check.timer` runs `bresca-updates@check` 5 minutes after
  boot and then every 6 hours (`image/src/bresca-cli/`). It only checks. Its
  result is `/run/bresca/updates/check.json`, readable without root.
- The applet only reads that file when it changes: `inotifywait`
  (inotify-tools) sleeps in the kernel until `check.json` is moved into
  place or removed, then the applet reads it and waits again.
- `update_available`: the icon shows;
  A successful install removes `check.json`, so
  the icon goes.
- With nothing to install, its popup has **Check for Updates**: it starts
  `bresca-updates@check` (`systemctl start --no-block`).
- The notification, event `updateAvailable` of `brescaos-updates.notifyrc`,
  comes when the applet sees a version for the first time in a session: when
  a check finds it, and at every login while it isn't installed, as a
  reminder. It stays until closed. Settings > Notifications > Applications >
  BrescaOS Updates configures or mutes it.

The system tray loads it on its own: `X-Plasma-NotificationArea` and
`EnabledByDefault` in `metadata.json`. In the development VM hot reload copies
a change and restarts plasmashell, which loads it.

`tests/` (`make test`) checks, in the BrescaOS image, that its files and
`inotifywait` are installed. Its QML only runs in Plasma: try it in the
development VM.
