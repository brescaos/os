# Hexa

A desktop theme for KDE Plasma 6, in two variants: **Hexa Light** and
**Hexa Dark**.

Both share one desktop layout: a top bar across the top edge with the
launcher (the system's logo, `LOGO` in `/etc/os-release`) and the active
app's menu (Plasma's global menu) on the left, the date and time in the middle and the system tray on the right, and a dock at
the bottom for the apps, as long as what it holds.

The top bar is Hexa's own panel containment, `com.brescaos.hexa.topbar`
(`plasmoids/`): it has no background, so Plasma draws neither a background nor
a shadow behind it, whatever the Plasma style, while the dock keeps the
style's. It lays its widgets in one row, in the order it keeps in its
settings (`AppletOrder`), each as tall as the bar and as wide as it asks, the
ones that ask to fill (the spacers, the global menu) sharing the free width;
with two spacers, what's between them (the clock) stays centred on the
screen, whatever the sides show: the global menu takes the room up to it.
Only when a side doesn't fit does the clock move away, just enough, up to
the other side; past that, the side is cut. Its widgets are
fixed: its panel template, `com.brescaos.hexa.topbar` (`layout-templates/`),
adds them, and the bar locks them, so none can be added, removed or
moved; each can still be configured (its context menu). The whole bar can be removed (its panel settings) and added again
(the desktop's Add Panel menu, Hexa Top Bar); the desktop layout loads the
same template.

The Plasma style is Hexa's, `hexa` (`desktoptheme/`): Breeze's, with opaque
popups and its own switches (`widgets/switch.svg`): Breeze's, with the
outlines of the handle and the track in the button text color at 30%, as the
Qt Quick Controls style's below.

The QtWidgets style is Hexa's, `Hexa` (`widgetstyle/`, C++, a Qt style
plugin): Breeze's, which it wraps (a `QProxyStyle`), without the line Breeze
draws along every scroll bar, next to the content
(`widgetstyle/widgets/scrollbar.cpp` leaves its strip out). Each control it
changes has its own file in `widgetstyle/widgets/`;
`widgetstyle/hexastyle.cpp` hands it each one. Qt Quick apps get it too: KDE's
desktop style draws their scroll bars with the QtWidgets style. Without
Breeze installed, it wraps Qt's default style.

Qt Quick apps (System Settings, Kirigami apps) have Hexa's Qt Quick Controls
style, `com.brescaos.hexa.style` (`style/`, QML): KDE's desktop style
(`org.kde.desktop`), which it falls back to for every control, except the
switches (`Switch`, `SwitchDelegate`). Their outlines mix 30% of the text
color into the background (`outlineContrast` in `style/SwitchIndicator.qml`)
instead of the color schemes' `frameContrast`, 8%, which keeps separators and
frames soft but leaves a switch's handle the color of a header behind it.
Qt uses it only where `QT_QUICK_CONTROLS_STYLE=com.brescaos.hexa.style` is
set; Plasma's widgets draw theirs with the Plasma style.

The window decoration is Hexa's, `com.brescaos.hexa.decoration`
(`decoration/`, C++, a KDecoration3 plugin): a title bar in the color
scheme's title bar colors, with the window's title centered on it in semibold (shortened
with "…" when it doesn't fit between the buttons), and the window's corners
rounder than Breeze's: `WINDOW_CORNER_RADIUS` in `decoration/decoration.cpp`,
8 px (Breeze's are 5), square when maximized. A 2 px line goes around
the window with the focus, the accent color 30% darker (`WINDOW_BORDER_DARKER_FACTOR`),
the same with light and dark color schemes; none when maximized. Every
window, with the focus or not, has the same black shadow below it
(`SHADOW_OFFSET_Y`, `SHADOW_BLUR` and `SHADOW_OPACITY`), one image that all
windows share. Its buttons are the app's
icon (the window menu), minimize, maximize and close, where the user puts
them (System Settings > Window Decorations > Titlebar Buttons); the Global
Themes put the icon on the left and the other three on the right. Other
buttons are left out.

The date and time are Hexa's clock, `com.brescaos.hexa.clock`
(`plasmoids/`, QML): the date, then the time in semibold, on one line, in
the user's language ("mié 14 nov  16:16" in Spanish). Plasma's own clock
draws both in the same weight. Its settings (its context menu, Configure
Hexa Clock...) pick the date's format from common ones, each shown as today
in it, and the time's: as the region has it, 12-hour or 24-hour. A click,
or Enter or Space, opens Plasma's calendar.

With no background, the bar's content is drawn on the wallpaper, so the bar
makes it white or black, whichever contrasts more (WCAG) with the wallpaper
behind it. `WallpaperContrast`, of Hexa's QML module `com.brescaos.hexa`
(`wallpapercontrast/`, C++: a panel's QML can't reach its screen's desktop),
tells it:

- It finds the desktop of the bar's screen and its wallpaper item, and
  follows them when the screens, the activity or the wallpaper plugin change.
- It looks at the wallpaper as Plasma draws it, so with any wallpaper plugin:
  one grab of the item, 256 px wide, of which it averages the luminance of
  the strip the bar covers. A light sky over dark ground is light for a bar
  at the top.
- It looks again only when the wallpaper says it changed: Plasma's wallpapers
  (image, slideshow, day and night, plain color) emit `accentColorChanged()`
  with every new image or color, as Plasma's accent color from the wallpaper
  needs. Nothing runs in between. A wallpaper plugin that doesn't emit it
  keeps the last result.
- The bar sets the color as its `Kirigami.Theme.textColor`, which its widgets
  inherit; their popups are windows of their own and keep the theme's colors.
  Until the first look, the theme's color.

| Source | Installed as |
| --- | --- |
| `color-schemes/HexaLight.colors`, `HexaDark.colors` | `/usr/share/color-schemes/` |
| `look-and-feel/<id>/contents/` | `/usr/share/plasma/look-and-feel/<id>/contents/` |
| `look-and-feel/<id>/metadata.json.in`, translated | `/usr/share/plasma/look-and-feel/<id>/metadata.json` |
| `look-and-feel/org.kde.plasma.desktop-layout.js`, for both | `/usr/share/plasma/look-and-feel/<id>/contents/layouts/` |
| `desktoptheme/hexa/dialogs/` | `/usr/share/plasma/desktoptheme/hexa/dialogs/` |
| `desktoptheme/hexa/widgets/` | `/usr/share/plasma/desktoptheme/hexa/widgets/` |
| `desktoptheme/hexa/metadata.json.in`, translated | `/usr/share/plasma/desktoptheme/hexa/metadata.json` |
| `icons/hexa-light/`, `icons/hexa-dark/` | `/usr/share/icons/hexa-light/`, `/usr/share/icons/hexa-dark/` |
| `icons/actions/` (with `16/`, drawn again for 16 px), `icons/status/`, for both | `/usr/share/icons/<theme>/actions/`, `/usr/share/icons/<theme>/status/` |
| `plasmoids/com.brescaos.hexa.topbar/contents/` | `/usr/share/plasma/plasmoids/com.brescaos.hexa.topbar/contents/` |
| `plasmoids/com.brescaos.hexa.topbar/metadata.json.in`, translated | `/usr/share/plasma/plasmoids/com.brescaos.hexa.topbar/metadata.json` |
| `plasmoids/com.brescaos.hexa.clock/contents/` | `/usr/share/plasma/plasmoids/com.brescaos.hexa.clock/contents/` |
| `plasmoids/com.brescaos.hexa.clock/metadata.json.in`, translated | `/usr/share/plasma/plasmoids/com.brescaos.hexa.clock/metadata.json` |
| `layout-templates/com.brescaos.hexa.topbar/contents/` | `/usr/share/plasma/layout-templates/com.brescaos.hexa.topbar/contents/` |
| `layout-templates/com.brescaos.hexa.topbar/metadata.json.in`, translated | `/usr/share/plasma/layout-templates/com.brescaos.hexa.topbar/metadata.json` |
| `style/` | `/usr/lib64/qt6/qml/com/brescaos/hexa/style/` |
| `wallpapercontrast/`, compiled (`CMakeLists.txt`) | `/usr/lib64/qt6/qml/com/brescaos/hexa/` |
| `decoration/`, compiled with `decoration/com.brescaos.hexa.decoration.json.in` translated | `/usr/lib64/qt6/plugins/org.kde.kdecoration3/com.brescaos.hexa.decoration.so` |
| `widgetstyle/`, compiled | `/usr/lib64/qt6/plugins/styles/hexastyle.so` |

`<id>` is `com.brescaos.hexa.light.desktop` or `com.brescaos.hexa.dark.desktop`.
Each `metadata.json.in`, and the decoration's JSON, is a template: the build fills in the version
(`VERSION.txt`) and each language's `Description` from `po/`. The clock's
settings are translated in Hexa's domain, `hexa` (`i18ndc`), not the one
Plasma would give the applet.

The QML module, the window decoration and the QtWidgets style are the only
compiled parts:
cmake, extra-cmake-modules, gcc-c++, qt6-qtbase-devel,
qt6-qtdeclarative-devel, libplasma-devel, kf6-kconfig-devel,
kf6-kcoreaddons-devel and kdecoration-devel to build them, and Plasma's own
libraries to run them. The decoration's metadata is compiled in translated:
CMake needs `TRANSLATED_METADATA_DIR`, the folder where
`image/build/compile_translations.sh` writes it. `tests/luminancetest.cpp`
checks which areas of an image count as dark, and `tests/scrollbarlinetest.cpp`
that the QtWidgets style leaves Breeze's line out (skipped without Breeze).

Once installed, choose it in System Settings > Colors & Themes > Global Theme,
or with `plasma-apply-lookandfeel -a com.brescaos.hexa.light.desktop`.
The Qt Quick Controls style takes `QT_QUICK_CONTROLS_STYLE=com.brescaos.hexa.style`
in the session's environment (for example in `~/.config/environment.d/`), from
the next login.
