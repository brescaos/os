/*
    SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2017 The Qt Company Ltd.

    SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-or-later
*/

// KDE's desktop style's Switch (kf6-qqc2-desktop-style) with Hexa's
// indicator; its position is KDE's.

import QtQuick
import org.kde.desktop as Desktop

Desktop.Switch {
    id: control

    indicator: SwitchIndicator {
        x: if (control.contentItem !== null && control.contentItem.width > 0) {
            return control.mirrored ?
                control.width - width - control.rightPadding : control.leftPadding
        } else {
            return control.leftPadding + (control.availableWidth - width) / 2
        }
        y: if (control.contentItem !== null
            && (control.contentItem instanceof Text || control.contentItem instanceof TextEdit)
            && control.contentItem.lineCount > 1) {
            return control.topPadding
        } else {
            return control.topPadding + Math.round((control.availableHeight - height) / 2)
        }
        control: control
    }
}
