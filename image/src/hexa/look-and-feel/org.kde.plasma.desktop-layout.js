// Hexa's desktop layout, the same for both variants: a top bar across the
// whole top edge, and a dock at the bottom for the apps.
//
// Plasma runs this script the first time a user's shell starts (no
// ~/.config/plasma-org.kde.plasma.desktop-appletsrc yet) and when a user
// applies the Global Theme with "Desktop and window layout" checked.

// Top bar: the whole top edge, made first so it takes the corners. Hexa's
// panel template (layout-templates/com.brescaos.hexa.topbar).
loadTemplate("com.brescaos.hexa.topbar");

// Dock: at the bottom, centred, floating as Plasma's default panel, and as
// long as its apps ("fit"): it grows and shrinks as apps open and close. The
// pinned and running apps.
var dock = new Panel;
dock.location = "bottom";
dock.alignment = "center";
dock.lengthMode = "fit";
dock.height = 44;
dock.floating = true;
dock.hiding = "none";
var tasks = dock.addWidget("org.kde.plasma.icontasks");
tasks.currentConfigGroup = ["General"];
tasks.writeConfig("launchers", [
    "applications:org.kde.dolphin.desktop",
    "applications:org.kde.konsole.desktop",
    "applications:systemsettings.desktop"
]);
