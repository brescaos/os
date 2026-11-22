#include "color.h"

#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{

// The palette's lightness and chroma: every hue has them in sRGB (up to 0.119
// at this lightness).
constexpr double paletteLightness = 0.7;
constexpr double paletteChroma = 0.11;
constexpr int paletteSize = 32;

struct Oklab {
    double lightness;
    double a;
    double b;
};

double linearFromSrgb(double channel)
{
    return channel <= 0.04045 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
}

double srgbFromLinear(double channel)
{
    return channel <= 0.0031308 ? channel * 12.92 : 1.055 * std::pow(channel, 1 / 2.4) - 0.055;
}

// Björn Ottosson's OKLab matrices.
Oklab oklabFromColor(const QColor &color)
{
    const double red = linearFromSrgb(color.redF());
    const double green = linearFromSrgb(color.greenF());
    const double blue = linearFromSrgb(color.blueF());
    const double l = std::cbrt(0.4122214708 * red + 0.5363325363 * green + 0.0514459929 * blue);
    const double m = std::cbrt(0.2119034982 * red + 0.6806995451 * green + 0.1073969566 * blue);
    const double s = std::cbrt(0.0883024619 * red + 0.2817188376 * green + 0.6299787005 * blue);
    return {.lightness = 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
            .a = 1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
            .b = 0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s};
}

struct LinearRgb {
    double red;
    double green;
    double blue;

    bool isInSrgb() const
    {
        return std::min({red, green, blue}) >= 0 && std::max({red, green, blue}) <= 1;
    }
};

LinearRgb linearRgbFromOklch(double lightness, double chroma, double hueDegrees)
{
    const double a = chroma * std::cos(hueDegrees * std::numbers::pi / 180);
    const double b = chroma * std::sin(hueDegrees * std::numbers::pi / 180);
    const double l = std::pow(lightness + 0.3963377774 * a + 0.2158037573 * b, 3);
    const double m = std::pow(lightness - 0.1055613458 * a - 0.0638541728 * b, 3);
    const double s = std::pow(lightness - 0.0894841775 * a - 1.2914855480 * b, 3);
    return {.red = 4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
            .green = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
            .blue = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s};
}

}

namespace WorkboxColor
{

std::optional<QColor> parse(const QString &text)
{
    static const QRegularExpression hexadecimal(QStringLiteral("^#([0-9a-fA-F]{3}|[0-9a-fA-F]{6})$"));
    static const QRegularExpression decimal(QStringLiteral("^\\s*(\\d{1,3})\\s*,\\s*(\\d{1,3})\\s*,\\s*(\\d{1,3})\\s*$"));
    if (hexadecimal.match(text).hasMatch()) {
        return QColor(text);
    }
    const QRegularExpressionMatch channels = decimal.match(text);
    if (!channels.hasMatch()) {
        return std::nullopt;
    }
    const int red = channels.captured(1).toInt();
    const int green = channels.captured(2).toInt();
    const int blue = channels.captured(3).toInt();
    if (std::max({red, green, blue}) > 255) {
        return std::nullopt;
    }
    return QColor(red, green, blue);
}

QList<QColor> palette()
{
    QList<QColor> colors;
    for (int index = 0; index < paletteSize; ++index) {
        colors.append(fromOklch(paletteLightness, paletteChroma, index * 360.0 / paletteSize));
    }
    return colors;
}

QColor fromOklch(double lightness, double maximumChroma, double hueDegrees)
{
    LinearRgb rgb = linearRgbFromOklch(lightness, maximumChroma, hueDegrees);
    if (!rgb.isInSrgb()) {
        // The most chroma sRGB has for that lightness and hue, by bisection.
        double inSrgb = 0;
        double outOfSrgb = maximumChroma;
        for (int step = 0; step < 32; ++step) {
            const double chroma = (inSrgb + outOfSrgb) / 2;
            (linearRgbFromOklch(lightness, chroma, hueDegrees).isInSrgb() ? inSrgb : outOfSrgb) = chroma;
        }
        rgb = linearRgbFromOklch(lightness, inSrgb, hueDegrees);
    }
    return QColor::fromRgbF(srgbFromLinear(rgb.red), srgbFromLinear(rgb.green), srgbFromLinear(rgb.blue));
}

double oklchHue(const QColor &color)
{
    const Oklab oklab = oklabFromColor(color);
    const double hueDegrees = std::atan2(oklab.b, oklab.a) * 180 / std::numbers::pi;
    return hueDegrees < 0 ? hueDegrees + 360 : hueDegrees;
}

double distance(const QColor &first, const QColor &second)
{
    const Oklab firstOklab = oklabFromColor(first);
    const Oklab secondOklab = oklabFromColor(second);
    return std::hypot(firstOklab.lightness - secondOklab.lightness, firstOklab.a - secondOklab.a, firstOklab.b - secondOklab.b);
}

QColor pickFromPalette(const QList<QColor> &colorsInUse)
{
    const QList<QColor> colors = palette();

    QList<QColor> freeColors;
    for (const QColor &color : colors) {
        if (!colorsInUse.contains(color)) {
            freeColors.append(color);
        }
    }
    if (freeColors.isEmpty()) {
        return *std::ranges::min_element(colors, {}, [&colorsInUse](const QColor &color) {
            return colorsInUse.count(color);
        });
    }
    // With no color in use, every distance is infinite: the first wins.
    return *std::ranges::max_element(freeColors, {}, [&colorsInUse](const QColor &color) {
        double nearest = INFINITY;
        for (const QColor &colorInUse : colorsInUse) {
            nearest = std::min(nearest, distance(color, colorInUse));
        }
        return nearest;
    });
}

QList<QColor> suggestions(const QList<QColor> &colorsInUse, int count)
{
    QList<QColor> inUse = colorsInUse;
    QList<QColor> suggested;
    // With every color in use, the least used can be one already suggested
    // (a tie goes to the palette's order): it's in use once more, until
    // another is the least used.
    while (suggested.size() < count) {
        const QColor picked = pickFromPalette(inUse);
        inUse.append(picked);
        if (!suggested.contains(picked)) {
            suggested.append(picked);
        }
    }
    return suggested;
}

}
