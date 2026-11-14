// Hexa Clock's settings pages: one, General. Its strings are in Hexa's
// translation domain (hexa), not the one Plasma would give the applet.

import QtQuick
import org.kde.plasma.configuration

ConfigModel {
    ConfigCategory {
        name: i18ndc("hexa", "@title", "General")
        icon: "preferences-system-time"
        source: "configGeneral.qml"
    }
}
