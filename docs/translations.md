# Translations

BrescaOS's own components (`image/src/`) are translated with
[gettext](https://www.gnu.org/software/gettext/) `.po` files, which any `.po`
editor opens. English is the language of
the code; today they're translated into Spanish (`es`) and Catalan (`ca`).
Each component is shown in the language the user picks in System Settings >
Region & Language (or `LANG` in a terminal), and in English when there's no
translation for it.

## Where they are

Each translated component has a `po/` folder:

| File | What it is |
|---|---|
| `po/<domain>.pot` | Its strings, extracted from the code by `make pot`. Never edited by hand |
| `po/<lang>.po` | Their translation into `<lang>`: what translators edit |

| Component | Domain | Strings from |
|---|---|---|
| `autotile` | `kcm_brescaos_autotile` | `i18n()` in `common/autotilelayout.cpp` and the KCM's QML, `i18nd()` in the applet's, and its three `*.json.in` (script, applet, KCM) |
| `bresca-cli` | `bresca` | Its scripts' `$"..."` strings |
| `distrobox` | `plasma_applet_com.brescaos.distrobox` | `i18n()` in its tray applet's QML and `applet/metadata.json.in` |
| `bresca-krunner` | `krunner_brescaos` | `i18n()` in its C++ and `krunner_brescaos.json.in` |
| `updates-applet` | `plasma_applet_com.brescaos.updates` | `i18n()` in its QML, `brescaos-updates.notifyrc.in` and `metadata.json.in` |
| `updates-kcm` | `kcm_brescaos_updates` | `i18n()` in its QML and `kcm_brescaos_updates.json.in` |
| `welcome` | `bresca-welcome` | `i18n()` in `main.cpp`, `KI18n.i18n()` in its QML, and its `.desktop.in` files |

## Translating

1. Open `image/src/<component>/po/<lang>.po` in Poedit.
2. Translate what's untranslated, and review what's marked as needing work
   (fuzzy: a string that changed in the code since it was translated).
3. Keep what the code fills in: `%s` in the CLI's strings and `%1` in the
   applet's, in the same order. The names of commands and options
   (`bresca updates download`, `--no-tls`) and the alignment of the help's
   columns stay as they are.
4. Save. Poedit doesn't need to compile it: the build does.

In the development VM the change shows within a second
([hot reload](vm.md#hot-reload)).

## Changing strings in the code

- The CLI's messages are `$"..."` strings, which bash translates when it reads
  them (`TEXTDOMAIN` and `noexpand_translation` in `lib/messages.sh`, sourced
  first by every script). Values go in as `%s`: `print_message`,
  `print_warning` and `print_error` take a format and its arguments, e.g.
  `print_error $"couldn't install the update: %s" "$install_error"`. Never a
  `$variable` inside `$"..."`, and no `\n` at the end: they add it. A help
  text is one `$"..."` string. What `--format=json` prints is never
  translated.
- The QML of the applet and of the System Settings module uses `i18n("…")`,
  with `%1`, `%2` for values; the welcome's, `KI18n.i18n("…")` (`import
  org.kde.ki18n`), and its C++ `i18n("…")`. Its `.desktop` files are
  `*.in` templates too. Autotiling has one domain for its KCM and its
  applet: the applet names it, `i18nd("kcm_brescaos_autotile", "…")`, and
  its metadata files are `*.in` templates too. The applet's `.notifyrc` and `metadata.json`
  and the module's `kcm_brescaos_updates.json` are English-only templates
  (`*.in`): the build adds the translations. The module's search keywords
  (`X-KDE-Keywords`) are one string, translated as a whole, comma-separated;
  the applets' (`Keywords`), the same with each keyword followed by a semicolon.

Then run `make pot`: it extracts the strings into the `.pot` and merges them
into every `.po`, where the new ones are untranslated, and translate them.

## The checks

`make check-translations` (part of `make check`, and so of the CI) fails
when:

- a `.pot` is stale (a string changed in the code without `make pot`);
- a `.po` is missing, or out of sync with the code (strings it lacks or no
  longer used): run `make pot`;
- a `.po` doesn't compile, or has untranslated or fuzzy strings.

`make pot` and `make check-translations` need gettext and jq. `make -k check`
runs both checks even when the scripts fail, as the CI does.

## Adding a language

1. Add it to `TRANSLATION_LANGUAGES` in
   [`scripts/settings/settings.sh`](../scripts/settings/settings.sh).
2. Run `make pot`: it creates `po/<lang>.po` in every component.
3. Translate them.
4. Add its glibc langpack (`glibc-langpack-<lang>`) to `image/Containerfile`,
   so the system can use the language.

## Translating a new component

1. Create its `po/` folder with an empty `<domain>.pot`, `<domain>` being the
   gettext domain its code uses.
2. Add a rule for it to `extract_strings` in
   [`scripts/translations.sh`](../scripts/translations.sh) (which files, which
   language, which keywords), and run `make pot`.
3. Anything other than `.mo` files (a template the build fills in) goes in
   [`image/build/compile_translations.sh`](../image/build/compile_translations.sh).

## How it's built

The first stage of `image/Containerfile` (`translations`, on
`quay.io/fedora/fedora` with gettext and jq) runs
[`image/build/compile_translations.sh`](../image/build/compile_translations.sh) with `src/`
mounted. It writes, laid out as in the image:

- every `po/<lang>.po` compiled to
  `/usr/share/locale/<lang>/LC_MESSAGES/<domain>.mo`;
- the applet's `brescaos-updates.notifyrc`, with `Name[<lang>]=` and
  `Comment[<lang>]=` lines (`msgfmt --desktop`);
- the applet's `metadata.json`, with `"Name[<lang>]"`,
  `"Description[<lang>]"` and `"Keywords[<lang>]"` (jq);
- the welcome's `.desktop` files, for the application menu and autostart,
  with `Name[<lang>]=`, `Comment[<lang>]=` and `Keywords[<lang>]=` lines
  (`msgfmt --desktop`);
- the autotiling KWin script's and tray applet's `metadata.json`, as the
  updates applet's.

And, apart, the System Settings modules' `kcm_brescaos_updates.json` and
`kcm_brescaos_autotile.json`, and the KRunner plugin's
`krunner_brescaos.json`, with `"Name[<lang>]"`, `"Description[<lang>]"`
and `"X-KDE-Keywords[<lang>]"`: System Settings and KRunner read them from
the plugin's own metadata, so the `updates-kcm-build`, `autotile-build` and
`bresca-krunner-build` stages compile them into the plugins.

Every one of those metadata files also gets the version in its
component's `VERSION.txt`.

The image copies only that result: gettext isn't in it. The development VM's
hot reload runs the same script.
