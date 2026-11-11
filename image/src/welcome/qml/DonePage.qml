// Last page.
import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.ki18n

StepPage {
    title: KI18n.i18n("You're all set")
    text: KI18n.i18n("Enjoy BrescaOS. You can open this welcome again at any time from the application menu.")

    header: Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        width: Kirigami.Units.gridUnit * 4
        height: width
        radius: width / 2
        color: Kirigami.Theme.highlightColor

        Kirigami.Icon {
            anchors.centerIn: parent
            width: parent.width * 0.55
            height: width
            source: "checkmark"
            isMask: true
            color: Kirigami.Theme.highlightedTextColor
        }
    }
}
