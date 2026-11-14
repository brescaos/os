// Hexa Clock's General settings: the date's format, picked from common ones
// (each shown as today's date in it), and the time's (as the region has it,
// 12-hour or 24-hour), with a preview of both. Its strings are in Hexa's
// translation domain (hexa), not the one Plasma would give the applet.

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

import "formats.js" as Formats

KCM.SimpleKCM {
    id: page

    property string cfg_dateFormat
    property int cfg_timeFormat
    // Plasma also sets each setting's default; unused here.
    property string cfg_dateFormatDefault
    property int cfg_timeFormatDefault

    // The examples' and preview's moment; it doesn't tick, it shows formats.
    readonly property date now: new Date()

    // The date formats offered, in Qt's date format syntax; "" is the
    // region's short date.
    readonly property var dateFormats: [
        "ddd d MMM",    // mié 28 oct
        "dddd d MMMM",  // miércoles 28 octubre
        "d MMM",        // 28 oct
        "",             // the region's: 28/10/26
        "dd/MM/yy",     // 28/10/26
        "dd/MM/yyyy",   // 28/10/2026
        "MM/dd/yy",     // 10/28/26
        "yyyy-MM-dd"    // 2026-10-28
    ]

    Kirigami.FormLayout {
        QQC2.ComboBox {
            Kirigami.FormData.label: i18ndc("hexa", "@label:listbox", "Date format:")
            model: page.dateFormats.map(format => format === ""
                ? i18ndc("hexa", "@item:inlistbox date format, %1 an example", "As the region has it (%1)", Formats.dateText(Qt.locale(), page.now, format))
                : Formats.dateText(Qt.locale(), page.now, format))
            currentIndex: page.dateFormats.indexOf(page.cfg_dateFormat)
            onActivated: index => page.cfg_dateFormat = page.dateFormats[index]
        }

        QQC2.ComboBox {
            Kirigami.FormData.label: i18ndc("hexa", "@label:listbox", "Time format:")
            model: [
                i18ndc("hexa", "@item:inlistbox time format", "As the region has it"),
                i18ndc("hexa", "@item:inlistbox time format", "12-hour"),
                i18ndc("hexa", "@item:inlistbox time format", "24-hour")
            ]
            currentIndex: page.cfg_timeFormat
            onActivated: index => page.cfg_timeFormat = index
        }

        QQC2.Label {
            Kirigami.FormData.label: i18ndc("hexa", "@label", "Preview:")
            text: Formats.dateText(Qt.locale(), page.now, page.cfg_dateFormat) + "  "
                + Qt.locale().toString(page.now, Formats.timeFormat(Qt.locale(), page.cfg_timeFormat))
            textFormat: Text.PlainText
        }
    }
}
