# BrescaOS Updates (System Settings)

A System Settings module, **BrescaOS Updates**, under System Administration.
Its page is empty for now.

| Source | In the image |
| --- | --- |
| `kcm.cpp`, `ui/` and `kcm_brescaos_updates.json.in`, translated | `/usr/lib64/qt6/plugins/plasma/kcms/systemsettings/kcm_brescaos_updates.so` |
| `kcm_brescaos_updates.json.in`, translated | `/usr/share/applications/kcm_brescaos_updates.desktop` |
| `po/<lang>.po` | `/usr/share/locale/<lang>/LC_MESSAGES/kcm_brescaos_updates.mo` |

It's C++ (KF6 KCMUtils) because System Settings only loads modules compiled
as a plugin; `kcm.cpp` is only that plugin, and the page is `ui/main.qml`,
compiled in. `image/Containerfile` compiles it in its own stage,
`updates-kcm-build`, so the compilers and `-devel` packages stay out of the
image.

Its texts are translated ([`docs/translations.md`](../../../docs/translations.md)),
domain `kcm_brescaos_updates` (its plugin id): `i18n()` in the QML, and
the English-only template `kcm_brescaos_updates.json.in` for its name,
description and search keywords in System Settings. System Settings reads
those from the metadata compiled into the module, so
`image/build/compile_translations.sh` fills in the template with the
translations of `po/` and the build compiles the result in
(`TRANSLATED_METADATA_DIR`).
The `.desktop`, which lets the launcher's search find the module, is
generated from it too.

In the development VM hot reload compiles it again when its sources or its
translations change, and reopens System Settings on it if it's open.

`tests/` (`make test`) checks, in the BrescaOS image, that the module, its
`.desktop` and its translations are installed, and that its metadata and its
`.desktop` are translated.
