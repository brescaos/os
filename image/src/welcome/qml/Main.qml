// The welcome window: the welcome page, every step and the last page, one
// after another, with Skip / Back / Next at the bottom.
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

QQC2.ApplicationWindow {
    id: root

    // The steps, in order: one folder each in steps/ (README, "Adding a step").
    property list<url> steps: []

    readonly property list<url> pages: [Qt.resolvedUrl("WelcomePage.qml"), ...steps, Qt.resolvedUrl("DonePage.qml")]
    readonly property bool atStart: view.currentIndex === 0
    readonly property bool atEnd: view.currentIndex === pages.length - 1
    readonly property bool rtl: Application.layoutDirection === Qt.RightToLeft
    readonly property alias pageView: view
    readonly property alias backButton: backButton
    readonly property alias nextButton: nextButton

    title: KI18n.i18n("Welcome to BrescaOS")
    width: Kirigami.Units.gridUnit * 40
    height: Kirigami.Units.gridUnit * 34
    minimumWidth: Kirigami.Units.gridUnit * 24
    minimumHeight: Kirigami.Units.gridUnit * 28
    visible: true
    color: Kirigami.Theme.backgroundColor

    function next() {
        if (atEnd) {
            Qt.quit();
        } else {
            view.incrementCurrentIndex();
        }
    }

    function back() {
        if (atStart) {
            Qt.quit();
        } else {
            view.decrementCurrentIndex();
        }
    }

    Shortcut {
        sequences: [StandardKey.Cancel]
        onActivated: Qt.quit()
    }

    QQC2.SwipeView {
        id: view
        anchors.fill: parent
        interactive: false
        clip: true

        Repeater {
            model: root.pages

            Loader {
                required property url modelData
                source: modelData
            }
        }
    }

    footer: QQC2.ToolBar {
        position: QQC2.ToolBar.Footer
        padding: Kirigami.Units.largeSpacing * 2

        RowLayout {
            anchors.fill: parent

            QQC2.Button {
                id: backButton
                text: root.atStart ? KI18n.i18n("Skip") : KI18n.i18n("Back")
                icon.name: root.atStart ? "" : (root.rtl ? "go-next" : "go-previous")
                flat: true
                onClicked: root.back()
            }

            Item {
                Layout.fillWidth: true
                implicitHeight: indicator.implicitHeight

                QQC2.PageIndicator {
                    id: indicator
                    anchors.centerIn: parent
                    count: view.count
                    currentIndex: view.currentIndex
                }
            }

            QQC2.Button {
                id: nextButton
                text: root.atEnd ? KI18n.i18n("Start using BrescaOS") : KI18n.i18n("Next")
                icon.name: root.atEnd ? "dialog-ok" : (root.rtl ? "go-previous" : "go-next")
                highlighted: true
                focus: true
                onClicked: root.next()
            }
        }
    }
}
