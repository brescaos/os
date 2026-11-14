// Hexa's clock for the top bar: the date, then the time in semibold, on one
// line, in the user's language: "mié 28 oct  20:21" in Spanish, "Wed 28 Oct
// 20:21" in English by default. Both formats are settings (config/main.xml,
// configGeneral.qml): the date's picked from common ones, the time as the
// locale has it, 12-hour or 24-hour. A click, or Enter or Space when it has
// the focus, opens Plasma's calendar.
// Plasma's own clock can't do this: it draws the time and the date in the
// same weight, and always shows the time.

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.workspace.calendar as PlasmaCalendar
import org.kde.plasma.clock
import org.kde.kirigami as Kirigami

import "formats.js" as Formats

PlasmoidItem {
    id: root

    Clock {
        id: clock
        // no time zone: it follows the system's
    }

    readonly property string dateText: Formats.dateText(Qt.locale(), clock.dateTime, Plasmoid.configuration.dateFormat)
    readonly property string timeText: Qt.locale().toString(clock.dateTime, Formats.timeFormat(Qt.locale(), Plasmoid.configuration.timeFormat))

    preferredRepresentation: compactRepresentation
    toolTipMainText: Qt.locale().toString(clock.dateTime, "dddd")
    toolTipSubText: Qt.formatDate(clock.dateTime, Qt.locale(), Locale.LongFormat)

    compactRepresentation: MouseArea {
        Layout.minimumWidth: labels.implicitWidth
        Layout.preferredWidth: labels.implicitWidth

        activeFocusOnTab: true
        Accessible.role: Accessible.Button
        Accessible.name: root.dateText + " " + root.timeText
        Accessible.onPressAction: root.expanded = !root.expanded

        onClicked: root.expanded = !root.expanded
        Keys.onPressed: event => {
            if (event.key === Qt.Key_Space || event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                root.expanded = !root.expanded;
                event.accepted = true;
            }
        }

        RowLayout {
            id: labels
            anchors.centerIn: parent
            spacing: Kirigami.Units.largeSpacing

            PlasmaComponents.Label {
                text: root.dateText
                textFormat: Text.PlainText
            }
            PlasmaComponents.Label {
                text: root.timeText
                font.weight: Font.DemiBold
                font.features: ({ "tnum": 1 })
                textFormat: Text.PlainText
            }
        }
    }

    fullRepresentation: PlasmaCalendar.MonthView {
        Layout.minimumWidth: Kirigami.Units.gridUnit * 20
        Layout.minimumHeight: Kirigami.Units.gridUnit * 20
        today: clock.dateTime
        firstDayOfWeek: Qt.locale().firstDayOfWeek
        borderOpacity: 0.25
    }
}
