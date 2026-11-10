# BrescaOS Updates (System Settings)

A System Settings module, **BrescaOS Updates**, under System Administration:
the installed version and where updates come from, checking for, downloading
and installing a new one, restarting into it, and going back to the previous
one.

| Source | In the image |
| --- | --- |
| `kcm.h`, `kcm.cpp`, `ui/` and `kcm_brescaos_updates.json.in`, translated | `/usr/lib64/qt6/plugins/plasma/kcms/systemsettings/kcm_brescaos_updates.so` |
| `kcm_brescaos_updates.json.in`, translated | `/usr/share/applications/kcm_brescaos_updates.desktop` |
| `po/<lang>.po` | `/usr/share/locale/<lang>/LC_MESSAGES/kcm_brescaos_updates.mo` |

It's C++ (KF6 KCMUtils) because System Settings only loads modules compiled
as a plugin; the page is `ui/main.qml`, compiled in. `image/Containerfile`
compiles it in its own stage, `updates-kcm-build`, so the compilers and
`-devel` packages stay out of the image.

## How it works

It never runs bootc: it starts the `bresca-updates@<operation>` services of
the CLI (`image/src/bresca-cli/`) and shows the results they leave in
`/run/bresca/updates/`, as the tray applet does.

- `kcm.cpp` (`UpdatesKcm`) reads every `<name>.json` there into `results`
  and tells which operations are running (`runningOperations`); `ui/main.qml`
  decides from them what to show. It reads them only when the folder changes
  (inotify, `QFileSystemWatcher`), and asks systemd (D-Bus,
  `ListUnitsByPatterns`) which operations are running then: nothing runs
  while nothing changes.
- It starts an operation with `systemctl start bresca-updates@<operation>`,
  only from a fixed list. That returns when the operation ends, so its end
  needs no polling. `status` and `check` start without a password
  (`bresca-updates.rules`); `download`, `download-install`, `install` and
  `rollback` ask for an administrator's password (polkit's dialog, kept a few
  minutes). Cancelling it changes nothing.
- When the page opens it runs `status` (the versions and the image updates
  come from: `bresca updates status --format=json`), and `check` only if
  there's no check since the boot. `status` runs again whenever a download,
  an install or a rollback leaves a new result.
- From a registry allowed without TLS, downloading (Download and Install,
  Download Only) asks first, as the CLI's `--no-tls` does: the services
  don't check it.
- Go Back is only offered with nothing staged: `bresca-updates@rollback`
  would delete it.
- Restart Now opens Plasma's restart prompt (`org.kde.LogoutPrompt`).
- The channel has only Stable for now, and changes nothing.

It has no Apply, Reset or Defaults: everything applies at once.

## Translations

Its texts are translated ([`docs/translations.md`](../../../docs/translations.md)),
domain `kcm_brescaos_updates` (its plugin id): `i18n()` in the QML, and
the English-only template `kcm_brescaos_updates.json.in` for its name,
description and search keywords in System Settings. System Settings reads
those from the metadata compiled into the module, so
`image/build/compile_translations.sh` fills in the template with the
translations of `po/` and the build compiles the result in
(`TRANSLATED_METADATA_DIR`).
The `.desktop`, which lets the launcher's search find the module, is
generated from it too. bootc's errors are shown as bootc writes them.

## Development

In the development VM hot reload compiles it again when its sources or its
translations change, and reopens System Settings on it if it's open.
`.clangd` gives clangd the include folders without a build.

`tests/` (`make test`) checks, in the BrescaOS image, that the module, its
`.desktop` and its translations are installed, and that its metadata and its
`.desktop` are translated. What it shows and starts only runs in System
Settings: try it in the development VM.
