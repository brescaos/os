// The layouts to pick one, the KCM's and the tray applet's: one card a row, a radio
// button for the current one, then its symbol beside its name and description. The whole card
// picks it.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: list

    // AutotileLayout's, and the name of the one to show as current.
    required property var layouts
    required property string current

    signal picked(string name)

    spacing: Kirigami.Units.smallSpacing

    Repeater {
        model: list.layouts

        delegate: QQC2.AbstractButton {
            id: card

            required property var modelData
            readonly property bool selected: modelData.name === list.current

            Layout.fillWidth: true
            padding: Kirigami.Units.largeSpacing
            hoverEnabled: true
            text: modelData.label
            Accessible.role: Accessible.RadioButton
            Accessible.checked: selected
            Accessible.description: modelData.description
            onClicked: list.picked(modelData.name)

            background: Rectangle {
                radius: Kirigami.Units.gridUnit * 0.75
                color: card.hovered ? Qt.alpha(Kirigami.Theme.textColor, 0.1)
                    : Qt.alpha(Kirigami.Theme.textColor, 0.05)
                border.width: 1
                border.color: card.visualFocus ? Kirigami.Theme.highlightColor
                    : Qt.alpha(Kirigami.Theme.textColor, 0.15)
            }

            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing

                QQC2.RadioButton {
                    // The card takes the keyboard and speaks for it.
                    focusPolicy: Qt.NoFocus
                    Accessible.ignored: true
                    checked: card.selected
                    // Each is in its own card, so nothing unchecks the others but this.
                    autoExclusive: false
                    onClicked: {
                        list.picked(card.modelData.name);
                        checked = Qt.binding(() => card.selected);
                    }
                }

                LayoutIcon {
                    Layout.preferredWidth: Kirigami.Units.iconSizes.large
                    Layout.preferredHeight: Kirigami.Units.iconSizes.large
                    file: card.modelData.icon
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    QQC2.Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        font.weight: Font.DemiBold
                        text: card.text
                        color: Kirigami.Theme.textColor
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        opacity: 0.7
                        text: card.modelData.description
                        color: Kirigami.Theme.textColor
                    }
                }
            }
        }
    }
}
