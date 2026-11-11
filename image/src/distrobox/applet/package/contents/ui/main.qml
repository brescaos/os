// The Distrobox tray applet (README.md): Distrobox's symbol, monochrome like
// the tray's others, with the number of running distroboxes when there's any.
// Clicking it lists every distrobox, the stopped ones muted, each with the
// symbol in its color and buttons to start or stop it and to remove it, which
// asks in the row itself; a spinner takes a button's place while its operation
// runs, and the row's other button waits. The list and the actions come from
// com.brescaos.distrobox (distroboxmodel.h). Translated with i18n(): Plasma
// gives the applet its domain, plasma_applet_com.brescaos.distrobox.

// The delegates read the outer ids (distroboxes).
pragma ComponentBehavior: Bound

// i18n comes from the KLocalizedQmlContext that Plasma sets on this
// component, which qmllint can't see.
// qmllint disable unqualified

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid
import com.brescaos.distrobox

PlasmoidItem {
    id: root

    readonly property url symbol: Qt.resolvedUrl("../icons/distrobox-symbolic.svg")

    component RowButton: PlasmaComponents.ToolButton {
        display: QQC2.AbstractButton.IconOnly
        PlasmaComponents.ToolTip.text: text
        PlasmaComponents.ToolTip.visible: hovered
    }

    // In place of a RowButton while its operation runs.
    component RowSpinner: PlasmaComponents.BusyIndicator {
        anchors.centerIn: parent
        width: Kirigami.Units.iconSizes.smallMedium
        height: width
        running: visible
    }

    DistroboxModel {
        id: distroboxes
    }

    Plasmoid.status: PlasmaCore.Types.ActiveStatus
    toolTipMainText: Plasmoid.title
    toolTipSubText: i18np("%1 distrobox running", "%1 distroboxes running", distroboxes.runningCount)

    // The count in a badge on its bottom right corner, none at 0.
    compactRepresentation: MouseArea {
        hoverEnabled: true
        onClicked: root.expanded = !root.expanded

        Kirigami.Icon {
            id: compactSymbol
            anchors.fill: parent
            source: root.symbol
            isMask: true
            color: Kirigami.Theme.textColor
            active: parent.containsMouse
        }

        // Small enough to leave the symbol readable, with a ring of the
        // panel's color that sets it apart from the symbol's lines.
        Rectangle {
            visible: distroboxes.runningCount > 0
            anchors.right: compactSymbol.right
            anchors.bottom: compactSymbol.bottom
            height: Math.round(compactSymbol.height * 0.4)
            width: Math.max(height, badgeLabel.implicitWidth + Kirigami.Units.smallSpacing)
            radius: height / 2
            color: Kirigami.Theme.highlightColor
            border.color: Kirigami.Theme.backgroundColor
            border.width: 1

            PlasmaComponents.Label {
                id: badgeLabel
                anchors.centerIn: parent
                text: distroboxes.runningCount > 9 ? "9+" : distroboxes.runningCount
                color: Kirigami.Theme.highlightedTextColor
                font.pixelSize: Math.round(parent.height * 0.7)
                font.weight: Font.DemiBold
            }
        }
    }

    fullRepresentation: PlasmaExtras.Representation {
        Layout.minimumWidth: Kirigami.Units.gridUnit * 16
        Layout.minimumHeight: Kirigami.Units.gridUnit * 10
        Layout.preferredWidth: Kirigami.Units.gridUnit * 20
        Layout.preferredHeight: Kirigami.Units.gridUnit * 16
        collapseMarginsHint: true

        header: PlasmaExtras.PlasmoidHeading {
            PlasmaExtras.Heading {
                anchors.fill: parent
                level: 1
                text: Plasmoid.title
            }
        }

        PlasmaComponents.ScrollView {
            anchors.fill: parent

            ListView {
                id: list
                model: distroboxes
                clip: true

                delegate: PlasmaComponents.ItemDelegate {
                    id: row

                    required property string name
                    required property bool running
                    // Konsole's color for its tab.
                    required property color color
                    // DistroboxModel.None, Starting, Stopping or Removing.
                    required property int operation
                    // The last operation, if it failed, and what the command
                    // said.
                    required property int failedOperation
                    required property string errorDetails
                    // Remove was clicked: the row asks.
                    property bool isConfirmingRemoval: false

                    readonly property bool isBusy: operation !== DistroboxModel.None

                    width: ListView.view.width
                    // The row itself does nothing: its buttons do.

                    contentItem: ColumnLayout {
                        spacing: Kirigami.Units.smallSpacing

                        RowLayout {
                            spacing: Kirigami.Units.smallSpacing
                            Layout.fillWidth: true

                            Kirigami.Icon {
                                source: root.symbol
                                isMask: true
                                color: row.color
                                opacity: row.running ? 1 : 0.7
                                Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                                Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                            }

                            PlasmaComponents.Label {
                                text: row.name
                                opacity: row.running ? 1 : 0.7
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Item {
                                implicitWidth: startStop.implicitWidth
                                implicitHeight: startStop.implicitHeight

                                RowButton {
                                    id: startStop
                                    anchors.fill: parent
                                    visible: row.operation !== DistroboxModel.Starting && row.operation !== DistroboxModel.Stopping
                                    enabled: !row.isBusy
                                    icon.name: row.running ? "media-playback-stop" : "media-playback-start"
                                    text: row.running ? i18n("Stop") : i18n("Start")
                                    onClicked: row.running ? distroboxes.stop(row.name) : distroboxes.start(row.name)
                                }

                                RowSpinner {
                                    visible: !startStop.visible
                                }
                            }

                            Item {
                                implicitWidth: removeButton.implicitWidth
                                implicitHeight: removeButton.implicitHeight

                                RowButton {
                                    id: removeButton
                                    anchors.fill: parent
                                    visible: row.operation !== DistroboxModel.Removing
                                    enabled: !row.isBusy
                                    icon.name: "edit-delete"
                                    text: i18n("Remove")
                                    checkable: true
                                    checked: row.isConfirmingRemoval
                                    onClicked: row.isConfirmingRemoval = !row.isConfirmingRemoval
                                }

                                RowSpinner {
                                    visible: !removeButton.visible
                                }
                            }
                        }

                        // The last operation's failure, until the next one.
                        PlasmaComponents.Label {
                            visible: row.failedOperation !== DistroboxModel.None
                            text: {
                                switch (row.failedOperation) {
                                case DistroboxModel.Starting:
                                    return i18n("Couldn't start it.");
                                case DistroboxModel.Stopping:
                                    return i18n("Couldn't stop it.");
                                case DistroboxModel.Removing:
                                    return i18n("Couldn't remove it.");
                                }
                                return "";
                            }
                            color: Kirigami.Theme.negativeTextColor
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        PlasmaComponents.Label {
                            visible: row.failedOperation !== DistroboxModel.None && row.errorDetails !== ""
                            text: row.errorDetails
                            color: Kirigami.Theme.disabledTextColor
                            font: Kirigami.Theme.smallFont
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        // The confirmation, in place of a dialog.
                        PlasmaComponents.Label {
                            visible: row.isConfirmingRemoval
                            text: i18n("Remove the distrobox “%1”? Everything installed in it will be lost. Your home folder is not affected.", row.name)
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            visible: row.isConfirmingRemoval
                            spacing: Kirigami.Units.smallSpacing
                            Layout.alignment: Qt.AlignRight

                            PlasmaComponents.Button {
                                icon.name: "edit-delete"
                                text: i18n("Remove")
                                onClicked: {
                                    row.isConfirmingRemoval = false;
                                    distroboxes.remove(row.name);
                                }
                            }

                            PlasmaComponents.Button {
                                icon.name: "dialog-cancel"
                                text: i18n("Cancel")
                                onClicked: row.isConfirmingRemoval = false
                            }
                        }
                    }
                }

                PlasmaExtras.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 4
                    visible: list.count === 0
                    text: i18n("No distroboxes")
                    explanation: i18n("Create one with “distrobox create <name>” in a terminal.")
                }
            }
        }
    }
}
