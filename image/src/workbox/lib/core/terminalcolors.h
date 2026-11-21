// A workbox's colors in the terminal (README.md), from its color's hue: a dark
// background, light text, and a light label for the prompt, so the text and
// the label read on the background at 7:1 or more (WCAG AAA) whatever the hue.
#pragma once

#include <QColor>
#include <QMap>

struct TerminalColors {
    QColor background;
    QColor text;
    QColor label;
};

TerminalColors terminalColorsFor(const QColor &workboxColor);

// What sets a terminal's background and text to colors' (OSC 11 and 10).
QString colorsSequence(const TerminalColors &colors);

// A login shell's environment for its prompt in the workbox named
// workboxName, "🧰 name [user@id dir]$ ", the name bold in the label's color:
// PS1 and WORKBOX_NAME.
QMap<QString, QString> promptEnvironment(const QString &workboxName, const TerminalColors &colors);
