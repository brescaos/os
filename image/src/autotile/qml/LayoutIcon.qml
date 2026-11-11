// A layout's symbol (icons/), monochrome in the text colour, like the tray's
// other symbols.
import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.Icon {
    // A file's name in icons/.
    required property string file

    source: Qt.resolvedUrl("icons/" + file)
    isMask: true
    color: Kirigami.Theme.textColor
}
