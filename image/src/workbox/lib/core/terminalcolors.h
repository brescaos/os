// A workbox's colors in the terminal (README.md), from its color's hue: a dark
// background, light text, and a light label for the prompt, so the text and
// the label read on the background at 7:1 or more (WCAG AAA) whatever the hue.
#pragma once

#include <QColor>

struct TerminalColors {
    QColor background;
    QColor text;
    QColor label;
};

TerminalColors terminalColorsFor(const QColor &workboxColor);
