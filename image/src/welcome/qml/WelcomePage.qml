// First page: what BrescaOS is, that it's alpha, and the Fedora notice.
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

StepPage {
    // The BrescaOS version, from main.cpp.
    title: KI18n.i18n("Welcome to BrescaOS %1", Application.version)
    text: KI18n.i18n("BrescaOS is a desktop operating system built on Fedora and KDE Plasma. The whole system is a single versioned image: an update installs a complete new version in one step, and if something goes wrong you can go back to the previous one. Your files and apps are kept across updates.")

    header: Kirigami.Icon {
        anchors.horizontalCenter: parent.horizontalCenter
        width: Kirigami.Units.gridUnit * 4
        height: width
        source: "help-about"
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: true
        type: Kirigami.MessageType.Warning
        text: KI18n.i18n("BrescaOS is in alpha. Expect bugs and changes between versions, and keep a backup of anything important.")
    }

    // Fedora's trademark guidelines require this notice, shown prominently,
    // from systems that combine Fedora with other software (README.md,
    // "Fedora notice").
    footer: QQC2.Label {
        objectName: "fedoraNotice"
        width: parent.width
        text: KI18n.i18n("BrescaOS is not provided or supported by the Fedora Project. Official, unmodified Fedora software is available at <a href=\"%1\">%1</a>.", "https://fedoraproject.org/")
        textFormat: Text.StyledText
        wrapMode: Text.WordWrap
        font.italic: true
        color: Kirigami.Theme.disabledTextColor
        onLinkActivated: link => Qt.openUrlExternally(link)

        HoverHandler {
            cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
    }
}
