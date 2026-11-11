// Layout shared by every page: an optional header (an icon, centred above the
// title), a title, a short text and the page's own content, centred and
// scrollable when the window is small, and an optional footer at the bottom.
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Flickable {
    id: page

    property string title
    property string text
    default property alias content: body.data
    // Items centred above the title; anchor them with
    // anchors.horizontalCenter: parent.horizontalCenter.
    property alias header: headerSlot.data
    // Items kept at the bottom of the page, below the content when it doesn't
    // fit; give them width: parent.width.
    property alias footer: footerSlot.data

    readonly property real margin: Kirigami.Units.gridUnit * 1.5
    readonly property real footerSpace: footerSlot.visible ? footerSlot.height + column.spacing : 0
    // Height the content needs; above the page's height, it scrolls.
    readonly property real neededHeight: column.implicitHeight + footerSpace + 2 * margin

    clip: true
    contentHeight: Math.max(height, neededHeight)
    boundsBehavior: Flickable.StopAtBounds

    QQC2.ScrollBar.vertical: QQC2.ScrollBar {}

    ColumnLayout {
        id: column
        width: Math.min(page.width - 2 * page.margin, Kirigami.Units.gridUnit * 34)
        x: (page.width - width) / 2
        y: Math.max(page.margin, (page.height - page.footerSpace - implicitHeight) / 2)
        spacing: Kirigami.Units.largeSpacing * 2

        Item {
            id: headerSlot
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            visible: children.length > 0
        }

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 1
            text: page.title
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        QQC2.Label {
            Layout.fillWidth: true
            visible: text.length > 0
            text: page.text
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            opacity: 0.8
        }

        ColumnLayout {
            id: body
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing * 2
        }
    }

    Item {
        id: footerSlot
        x: column.x
        y: page.contentHeight - page.margin - height
        width: column.width
        height: childrenRect.height
        visible: children.length > 0
    }
}
