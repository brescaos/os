// The autotiling tray applet: the current layout's symbol, or two free windows
// while it's off, and clicking it shows a switch for it and a link to its
// page in System Settings; on, the layouts as well. Its state and the list come
// from com.brescaos.autotile (common/autotilelayout.h, qml/ in
// image/src/autotile/). Translated with i18nd, in the KCM's domain: one .po for
// the whole component.

// i18n comes from the KLocalizedQmlContext that Plasma sets on this
// component, which qmllint can't see. The KI18n singleton needs it on the
// engine's root context, and Plasma doesn't put it there.
// qmllint disable unqualified

import QtQuick
import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid
import com.brescaos.autotile

PlasmoidItem {
    id: root

    readonly property var current: tiling.layouts.find(entry => entry.name === tiling.layout) || tiling.layouts[0]

    AutotileLayout {
        id: tiling
    }

    // Hidden if the KCM says so; kept while its popup is open.
    Plasmoid.status: tiling.showInTray || root.expanded ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.HiddenStatus
    toolTipMainText: Plasmoid.title
    toolTipSubText: tiling.enabled ? current.label : i18nd("kcm_brescaos_autotile", "Off")

    compactRepresentation: MouseArea {
        hoverEnabled: true
        onClicked: root.expanded = !root.expanded

        LayoutIcon {
            anchors.fill: parent
            file: tiling.enabled ? root.current.icon : "layout-none-symbolic.svg"
            active: parent.containsMouse
        }
    }

    fullRepresentation: PlasmaExtras.Representation {
        collapseMarginsHint: true

        PlasmaComponents.Switch {
            id: toggle
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Kirigami.Units.largeSpacing
            text: i18nd("kcm_brescaos_autotile", "Automatic tiling")
            checked: tiling.enabled
            onToggled: tiling.enabled = checked
        }

        // At the top, as tall as they need: the tray's popup is of a fixed size,
        // however little the applet shows.
        LayoutList {
            anchors.top: toggle.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Kirigami.Units.largeSpacing
            visible: tiling.enabled
            layouts: tiling.layouts
            current: tiling.layout
            onPicked: name => tiling.layout = name
        }

        PlasmaComponents.ToolButton {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.margins: Kirigami.Units.largeSpacing
            icon.name: "configure"
            text: i18nd("kcm_brescaos_autotile", "Manage in System Settings…")
            onClicked: {
                root.expanded = false;
                KCM.KCMLauncher.openSystemSettings("kcm_brescaos_autotile");
            }
        }
    }
}
