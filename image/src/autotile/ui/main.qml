// The module's page, a form like KDE's own: a switch for autotiling and, while it's
// on, a list of the layouts' cards and the gap; a checkbox for the tray icon. They wait for the module's Apply;
// Defaults is off, Binary tree, 8 px, no icon.
// com.brescaos.autotile is the installed QML module the tray applet uses.
//
// i18n comes from the context KCMUtils sets, which qmllint can't see.
// qmllint disable unqualified
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami
import com.brescaos.autotile

KCM.SimpleKCM {
    id: page

    // The module (kcm/kcm.cpp), which System Settings sets as the context
    // property kcm: its type isn't a QML one.
    readonly property var module: kcm
    property bool pendingEnabled: false
    property string pendingLayout: ""
    property int pendingGap: 0
    property bool pendingShowInTray: false

    function reset() {
        pendingEnabled = tiling.enabled;
        pendingLayout = tiling.layout;
        pendingGap = tiling.gap;
        pendingShowInTray = tiling.showInTray;
    }

    function apply() {
        // The layout first: the script reads it as it loads.
        if (pendingLayout !== tiling.layout) {
            tiling.layout = pendingLayout;
        }
        if (pendingGap !== tiling.gap) {
            tiling.gap = pendingGap;
        }
        tiling.enabled = pendingEnabled;
        tiling.showInTray = pendingShowInTray;
    }

    Component.onCompleted: reset()

    Binding {
        target: page.module
        property: "needsSave"
        value: page.pendingEnabled !== tiling.enabled || page.pendingLayout !== tiling.layout
            || page.pendingGap !== tiling.gap
            || page.pendingShowInTray !== tiling.showInTray
    }

    Binding {
        target: page.module
        property: "representsDefaults"
        value: !page.pendingEnabled && page.pendingLayout === "binaryTree"
            && page.pendingGap === tiling.defaultGap && !page.pendingShowInTray
    }

    Connections {
        target: page.module
        function onLoadRequested() { page.reset() }
        function onSaveRequested() { page.apply() }
        function onDefaultsRequested() {
            page.pendingEnabled = false;
            page.pendingLayout = "binaryTree";
            page.pendingGap = tiling.defaultGap;
            page.pendingShowInTray = false;
        }
    }

    // Changed from the tray applet or by Apply: what's saved is what counts.
    Connections {
        target: tiling
        function onEnabledChanged() { page.reset() }
        function onLayoutChanged() { page.reset() }
        function onGapChanged() { page.reset() }
        function onShowInTrayChanged() { page.reset() }
    }

    AutotileLayout {
        id: tiling
    }

    // Between groups of rows: an empty section title takes no room.
    component GroupSpacer: Item {
        implicitHeight: Kirigami.Units.gridUnit
    }

    topPadding: Kirigami.Units.gridUnit * 2
    // Down it already scrolls; sideways SimpleKCM doesn't unless told.
    horizontalScrollBarPolicy: QQC2.ScrollBar.AsNeeded

    Kirigami.FormLayout {
        QQC2.Switch {
            Kirigami.FormData.label: i18n("Automatic tiling:")
            text: i18n("Arrange windows automatically")
            checked: page.pendingEnabled
            onToggled: page.pendingEnabled = checked
        }

        QQC2.Label {
            Layout.maximumWidth: Kirigami.Units.gridUnit * 20
            wrapMode: Text.Wrap
            font: Kirigami.Theme.smallFont
            text: i18n("Windows take their place on their own and fill the screen. Turned off, they behave as usual.")
        }

        GroupSpacer {}

        LayoutList {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 28
            Kirigami.FormData.label: i18n("Layout:")
            Kirigami.FormData.labelAlignment: Qt.AlignTop
            visible: page.pendingEnabled
            layouts: tiling.layouts
            current: page.pendingLayout
            onPicked: name => page.pendingLayout = name
        }

        GroupSpacer {
            visible: page.pendingEnabled
        }

        QQC2.SpinBox {
            Kirigami.FormData.label: i18n("Gap between windows:")
            visible: page.pendingEnabled
            from: 0
            to: tiling.maxGap
            editable: true
            value: page.pendingGap
            textFromValue: value => i18n("%1 px", value)
            valueFromText: text => parseInt(text) || 0
            onValueModified: page.pendingGap = value
        }

        GroupSpacer {
            visible: page.pendingEnabled
        }

        QQC2.CheckBox {
            Kirigami.FormData.label: i18n("System tray:")
            text: i18n("Show the icon")
            checked: page.pendingShowInTray
            onToggled: page.pendingShowInTray = checked
        }

        QQC2.Label {
            Layout.maximumWidth: Kirigami.Units.gridUnit * 20
            wrapMode: Text.Wrap
            font: Kirigami.Theme.smallFont
            text: i18n("Turn tiling on and off and change its layout without opening System Settings.")
        }
    }
}
