// A workbox's color (README.md): the one given to create, or one of the
// palette's, as far as it can be from the colors in use.
#pragma once

#include <QColor>
#include <QList>

#include <optional>

namespace WorkboxColor
{

// #rgb, #rrggbb or r,g,b (each 0-255); nothing when it's none of them.
std::optional<QColor> parse(const QString &text);

// 32 hues, evenly spaced in OKLCH from 0, with the same lightness and chroma.
QList<QColor> palette();

// The color of that OKLCH lightness and hue, with maximumChroma or, where
// sRGB doesn't have it, the most sRGB has.
QColor fromOklch(double lightness, double maximumChroma, double hueDegrees);

// color's hue in OKLCH, in degrees from 0 to 360.
double oklchHue(const QColor &color);

// How different two colors look: their distance in OKLab.
double distance(const QColor &first, const QColor &second);

// The palette's color, among those not in use, whose nearest color in use is
// the farthest; with all of them in use, the least used. The first in the
// palette's order on a tie.
QColor pickFromPalette(const QList<QColor> &colorsInUse);

// count different colors of the palette (up to 32), the first
// pickFromPalette()'s, each next one what it would pick with the ones before
// in use too: far from the colors in use and from each other.
QList<QColor> suggestions(const QList<QColor> &colorsInUse, int count);

}
