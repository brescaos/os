# Hexa

A desktop theme for KDE Plasma 6, in two variants: **Hexa Light** and
**Hexa Dark**. Each is a Global Theme with its own color scheme: Breeze's, Hexa's
starting point, with Hexa's yellow accent (`#f2b33d`). Everything else (widget
style, window decoration, Plasma style, icons, cursors) is Breeze's.

| Source | Installed as |
| --- | --- |
| `color-schemes/HexaLight.colors`, `HexaDark.colors` | `/usr/share/color-schemes/` |
| `look-and-feel/<id>/contents/` | `/usr/share/plasma/look-and-feel/<id>/contents/` |
| `look-and-feel/<id>/metadata.json.in`, translated | `/usr/share/plasma/look-and-feel/<id>/metadata.json` |

`<id>` is `com.brescaos.hexa.light.desktop` or `com.brescaos.hexa.dark.desktop`.
`metadata.json.in` is a template: the build fills in the version
(`VERSION.txt`) and each language's `Description` from `po/`.

Once installed, choose it in System Settings > Colors & Themes > Global Theme,
or with `plasma-apply-lookandfeel -a com.brescaos.hexa.light.desktop`.
