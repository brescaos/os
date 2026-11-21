// workbox's tray applet (README.md): a toolbox, monochrome like the tray's
// others, with the number of running workboxes when there's any. Clicking it
// lists every workbox, the stopped ones muted, each with the toolbox in its
// color, its name, and buttons to start or stop it and to remove it, which
// asks in the row itself; a spinner takes a button's place while its operation
// runs, and the row's other button waits. The list and the actions come from
// com.brescaos.workbox (workboxmodel.h), which never blocks. Translated with
// i18nd() in workbox's domain, as its library's errors.

// The delegates read the outer ids (workboxes).
pragma ComponentBehavior: Bound

// i18nd comes from the KLocalizedQmlContext that Plasma sets on this
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
import com.brescaos.workbox

PlasmoidItem {
    id: root

    readonly property url symbol: Qt.resolvedUrl("../icons/workbox-symbolic.svg")

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

    WorkboxModel {
        id: workboxes
    }

    Plasmoid.status: PlasmaCore.Types.ActiveStatus
    toolTipMainText: Plasmoid.title
    toolTipSubText: i18ndp("workbox", "%1 workbox running", "%1 workboxes running", workboxes.runningCount)

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
        // panel's color that sets it apart from the symbol's lines. Raised
        // off the cell's bottom, which is the panel's: there it would touch
        // the edge of a window right below the panel.
        Rectangle {
            visible: workboxes.runningCount > 0
            anchors.right: compactSymbol.right
            anchors.bottom: compactSymbol.bottom
            anchors.bottomMargin: 4
            height: Math.round(compactSymbol.height * 0.4)
            width: Math.max(height, badgeLabel.implicitWidth + Kirigami.Units.smallSpacing)
            radius: height / 2
            color: Kirigami.Theme.highlightColor
            border.color: Kirigami.Theme.backgroundColor
            border.width: 1

            PlasmaComponents.Label {
                id: badgeLabel
                anchors.centerIn: parent
                text: workboxes.runningCount > 9 ? "9+" : workboxes.runningCount
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
                model: workboxes
                clip: true

                delegate: PlasmaComponents.ItemDelegate {
                    id: row

                    required property string id
                    required property string name
                    required property bool running
                    // The workbox's color.
                    required property color color
                    // WorkboxModel.None, Starting, Stopping or Removing.
                    required property int operation
                    // The last operation, if it failed, and why.
                    required property int failedOperation
                    required property string errorText
                    // Remove was clicked: the row asks.
                    property bool isConfirmingRemoval: false

                    readonly property bool isBusy: operation !== WorkboxModel.None

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

                            // The name, and the id when it's not the same.
                            ColumnLayout {
                                spacing: 0
                                opacity: row.running ? 1 : 0.7
                                Layout.fillWidth: true

                                PlasmaComponents.Label {
                                    text: row.name
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                PlasmaComponents.Label {
                                    visible: row.id !== row.name
                                    text: row.id
                                    color: Kirigami.Theme.disabledTextColor
                                    font: Kirigami.Theme.smallFont
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            Item {
                                implicitWidth: startStop.implicitWidth
                                implicitHeight: startStop.implicitHeight

                                RowButton {
                                    id: startStop
                                    anchors.fill: parent
                                    visible: row.operation !== WorkboxModel.Starting && row.operation !== WorkboxModel.Stopping
                                    enabled: !row.isBusy
                                    icon.name: row.running ? "media-playback-stop" : "media-playback-start"
                                    text: row.running ? i18nd("workbox", "Stop") : i18nd("workbox", "Start")
                                    onClicked: row.running ? workboxes.stop(row.id) : workboxes.start(row.id)
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
                                    visible: row.operation !== WorkboxModel.Removing
                                    enabled: !row.isBusy
                                    icon.name: "edit-delete"
                                    text: i18nd("workbox", "Remove")
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
                            visible: row.failedOperation !== WorkboxModel.None
                            text: {
                                switch (row.failedOperation) {
                                case WorkboxModel.Starting:
                                    return i18nd("workbox", "Couldn't start it.");
                                case WorkboxModel.Stopping:
                                    return i18nd("workbox", "Couldn't stop it.");
                                case WorkboxModel.Removing:
                                    return i18nd("workbox", "Couldn't remove it.");
                                }
                                return "";
                            }
                            color: Kirigami.Theme.negativeTextColor
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        PlasmaComponents.Label {
                            visible: row.failedOperation !== WorkboxModel.None && row.errorText !== ""
                            text: row.errorText
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
                            text: i18nd("workbox", "Remove the workbox “%1”? Everything in it and in its home will be lost.", row.name)
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            visible: row.isConfirmingRemoval
                            spacing: Kirigami.Units.smallSpacing
                            Layout.alignment: Qt.AlignRight

                            PlasmaComponents.Button {
                                icon.name: "edit-delete"
                                text: i18nd("workbox", "Remove")
                                onClicked: {
                                    row.isConfirmingRemoval = false;
                                    workboxes.remove(row.id);
                                }
                            }

                            PlasmaComponents.Button {
                                icon.name: "dialog-cancel"
                                text: i18nd("workbox", "Cancel")
                                onClicked: row.isConfirmingRemoval = false
                            }
                        }
                    }
                }

                PlasmaExtras.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 4
                    visible: list.count === 0
                    iconName: workboxes.problem !== "" ? "dialog-error" : ""
                    text: workboxes.problem !== "" ? i18nd("workbox", "Can't list the workboxes") : i18nd("workbox", "No workboxes")
                    explanation: workboxes.problem !== "" ? workboxes.problem : i18nd("workbox", "Create one with “workbox create <id>” in a terminal.")
                }
            }
        }
    }
}
