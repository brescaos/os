# Hexa

A desktop theme for KDE Plasma 6, in two variants: **Hexa Light** and
**Hexa Dark**. Each is a Global Theme with its own color scheme: Breeze's, Hexa's
starting point, with Hexa's yellow accent (`#f2b33d`). Everything else (widget
style, window decoration, Plasma style, icons, cursors) is Breeze's.

Both share one desktop layout: a top bar across the top edge with the
launcher (the system's logo, `LOGO` in `/etc/os-release`) and the active
app's menu (Plasma's global menu) on the left, the date and time in the middle and the system tray on the right, and a dock at
the bottom for the apps, as long as what it holds.

The top bar is Hexa's own panel containment, `com.brescaos.hexa.topbar`
(`plasmoids/`): it has no background, so Plasma draws neither a background nor
a shadow behind it, whatever the Plasma style, while the dock keeps the
style's. It lays its widgets in one row, in the order it keeps in its
settings (`AppletOrder`), each as tall as the bar and as wide as it asks, the
spacers sharing the free width; with two spacers, what's between them (the
clock) is centred on the screen, whatever the sides hold. Its widgets are
fixed: its panel template, `com.brescaos.hexa.topbar` (`layout-templates/`),
adds them, and the bar locks them, so none can be added, removed or
configured. The whole bar can be removed (its panel settings) and added again
(the desktop's Add Panel menu, Hexa Top Bar); the desktop layout loads the
same template.

| Source | Installed as |
| --- | --- |
| `color-schemes/HexaLight.colors`, `HexaDark.colors` | `/usr/share/color-schemes/` |
| `look-and-feel/<id>/contents/` | `/usr/share/plasma/look-and-feel/<id>/contents/` |
| `look-and-feel/<id>/metadata.json.in`, translated | `/usr/share/plasma/look-and-feel/<id>/metadata.json` |
| `look-and-feel/org.kde.plasma.desktop-layout.js`, for both | `/usr/share/plasma/look-and-feel/<id>/contents/layouts/` |
| `plasmoids/com.brescaos.hexa.topbar/contents/` | `/usr/share/plasma/plasmoids/com.brescaos.hexa.topbar/contents/` |
| `plasmoids/com.brescaos.hexa.topbar/metadata.json.in`, translated | `/usr/share/plasma/plasmoids/com.brescaos.hexa.topbar/metadata.json` |
| `layout-templates/com.brescaos.hexa.topbar/contents/` | `/usr/share/plasma/layout-templates/com.brescaos.hexa.topbar/contents/` |
| `layout-templates/com.brescaos.hexa.topbar/metadata.json.in`, translated | `/usr/share/plasma/layout-templates/com.brescaos.hexa.topbar/metadata.json` |

`<id>` is `com.brescaos.hexa.light.desktop` or `com.brescaos.hexa.dark.desktop`.
Each `metadata.json.in` is a template: the build fills in the version
(`VERSION.txt`) and each language's `Description` from `po/`.

Once installed, choose it in System Settings > Colors & Themes > Global Theme,
or with `plasma-apply-lookandfeel -a com.brescaos.hexa.light.desktop`.
