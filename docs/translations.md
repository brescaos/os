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
| `bresca-cli` | `bresca` | Its scripts' `$"..."` strings |
| `updates-applet` | `plasma_applet_com.brescaos.updates` | `i18n()` in its QML, `brescaos-updates.notifyrc.in` and `metadata.json.in` |

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
- The applet's QML uses `i18n("…")`, with `%1`, `%2` for values. Its
  `.notifyrc` and `metadata.json` are English-only templates
  (`*.notifyrc.in`, `metadata.json.in`): the build adds the translations.

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
   [`image/build/translations.sh`](../image/build/translations.sh).

## How it's built

The first stage of `image/Containerfile` (`translations`, on
`quay.io/fedora/fedora` with gettext and jq) runs
[`image/build/translations.sh`](../image/build/translations.sh) with `src/`
mounted. It writes, laid out as in the image:

- every `po/<lang>.po` compiled to
  `/usr/share/locale/<lang>/LC_MESSAGES/<domain>.mo`;
- the applet's `brescaos-updates.notifyrc`, with `Name[<lang>]=` and
  `Comment[<lang>]=` lines (`msgfmt --desktop`);
- the applet's `metadata.json`, with `"Name[<lang>]"` and
  `"Description[<lang>]"` (jq).

The image copies only that result: gettext isn't in it. The development VM's
hot reload runs the same script.
