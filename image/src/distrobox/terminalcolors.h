// A distrobox's colors in the terminal (README.md): Konsole's color for its
// tab, and from that color's hue, the terminal's background and the prompt's
// label.
#pragma once

#include <QColor>
#include <QStringList>

namespace TerminalColors
{

// The distrobox `distrobox enter <arguments>` enters, as distrobox-enter
// decides it: --name or the argument that isn't a flag, else
// $DBX_CONTAINER_NAME, else Distrobox's default.
QString enteredName(const QStringList &arguments, const QString &nameFromEnvironment);

// Konsole's color for the tab of a distrobox named `name`, with the color
// scheme's selection color as the accent it tints it with (Konsole 26.08's
// ContainerSessionState::colorForContainerKey).
QColor konsoleTabColor(const QString &name, const QColor &accent);

// `color`'s hue in OKLCH, in degrees from 0 to 360.
double oklchHue(const QColor &color);

struct Colors {
    QColor background;
    QColor label;
};

// A dark background (OKLCH lightness 0.28) and a light label (0.84) with the
// hue `hue`, each with as much chroma as sRGB has up to its cap.
Colors forHue(double hue);

// WCAG 2's contrast ratio between two colors, from 1 to 21.
double contrastRatio(const QColor &first, const QColor &second);

}
