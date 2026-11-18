/*
    SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2017 The Qt Company Ltd.
    SPDX-FileCopyrightText: 2025 Nate Graham <nate@kde.org>

    SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-or-later
*/

// KDE's desktop style's SwitchDelegate (kf6-qqc2-desktop-style) with Hexa's
// indicator; its position is KDE's.

import QtQuick
import QtQuick.Templates as T
import org.kde.desktop as Desktop

Desktop.SwitchDelegate {
    id: controlRoot

    indicator: SwitchIndicator {
        x: !controlRoot.mirrored ? controlRoot.horizontalPadding : controlRoot.width - width - controlRoot.horizontalPadding
        y: controlRoot.topPadding + (controlRoot.display === T.AbstractButton.TextUnderIcon ? 0 : ((controlRoot.availableHeight - height) / 2) )

        control: controlRoot
    }
}
