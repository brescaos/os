/*
    SPDX-FileCopyrightText: 2022 Tanbir Jishan <tantalising007@gmail.com>
    SPDX-FileCopyrightText: 2017 The Qt Company Ltd.

    SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-or-later
*/

// KDE's desktop style's SwitchIndicator (kf6-qqc2-desktop-style), with
// outlines of their own contrast: KDE's use the color scheme's
// frameContrast, which Hexa keeps low for soft separators and frames, and
// at that the handle and the track fade into a header's background. And no
// animation of the handle's color: its first one went from black, the color
// before the theme's arrives, to the theme's, at every opening.

import QtQuick
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami

Item {
    id: indicator
    implicitWidth: implicitHeight * 2
    implicitHeight: Kirigami.Units.gridUnit
    layer.enabled: control.opacity < 1.0

    property T.AbstractButton control
    property alias handle: handle

    // How much of the text color the outlines mix into the background.
    readonly property real outlineContrast: 0.30

    readonly property color handleBorderColor: (control.hovered || control.visualFocus)
        ? Kirigami.Theme.hoverColor
        : Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, outlineContrast)

    Kirigami.Theme.colorSet: Kirigami.Theme.Button
    Kirigami.Theme.inherit: false

    Rectangle {
        id: inactive

        anchors {
            fill: parent
            // margins so that the background is a bit shorter than the handle
            topMargin: Math.floor(parent.height / 6)
            bottomMargin: Math.floor(parent.height / 6)
        }

        radius: Math.round(height / 2)
        color: Qt.tint(Kirigami.Theme.textColor, Qt.alpha(Kirigami.Theme.backgroundColor, 0.9))
        border.color: indicator.control.checked ? Kirigami.Theme.highlightColor : indicator.handleBorderColor
    }

    Rectangle {
        anchors {
            left: inactive.left
            top: inactive.top
            bottom: inactive.bottom
            right: handle.right
        }

        radius: inactive.radius
        color: Qt.alpha(Kirigami.Theme.highlightColor, 0.5)
    }

    Rectangle {
        id: handle

        x: Math.max(0, Math.min(parent.width - width, indicator.control.visualPosition * parent.width - (width / 2)))

        anchors {
            top: parent.top
            bottom: parent.bottom
        }

        width: height
        radius: Math.floor(width / 2)
        color: Kirigami.Theme.backgroundColor
        border.color: indicator.handleBorderColor

        Behavior on x {
            enabled: !indicator.control.pressed && Kirigami.Units.shortDuration > 0
            SmoothedAnimation {
                duration: Kirigami.Units.shortDuration
            }
        }
    }
}
