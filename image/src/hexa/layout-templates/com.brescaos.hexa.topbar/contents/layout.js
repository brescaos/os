// Hexa's top bar, as a panel template: the desktop's "Add Panel" menu offers
// it, and Hexa's desktop layout loads it. The whole top edge: the launcher
// and the active app's menu (the global menu: apps that export their menus
// show them here instead of in their windows) on the left, the date and time
// (Hexa's clock, plasmoids/com.brescaos.hexa.clock) in the middle between two
// spacers, the system tray on the right.
// Hexa's own containment (plasmoids/com.brescaos.hexa.topbar): no background,
// so no shadow either, and its widgets locked: nothing can be added, removed
// or moved, only configured, and the whole bar removed.
var topBar = new Panel("com.brescaos.hexa.topbar");
topBar.location = "top";
topBar.lengthMode = "fill";
topBar.height = 26;
topBar.floating = false;
topBar.floatingApplets = true;
topBar.hiding = "none";

var launcher = topBar.addWidget("org.kde.plasma.kickoff");
// The system's logo: LOGO in os-release is an icon name, maybe quoted.
// Without one, Kickoff's default icon stays. KConfig keeps entries outside
// any group in "<default>".
var osLogo = ConfigFile("/etc/os-release", "<default>").readEntry("LOGO");
if (osLogo) {
    launcher.currentConfigGroup = ["General"];
    launcher.writeConfig("icon", String(osLogo).replace(/["']/g, ""));
}
topBar.addWidget("org.kde.plasma.appmenu");
topBar.addWidget("org.kde.plasma.panelspacer");
topBar.addWidget("com.brescaos.hexa.clock");
topBar.addWidget("org.kde.plasma.panelspacer");
topBar.addWidget("org.kde.plasma.systemtray");
