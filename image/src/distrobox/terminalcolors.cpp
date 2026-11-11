#include "terminalcolors.h"

#include <QHash>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace TerminalColors
{

namespace
{

using Rgb = std::array<double, 3>;

double toLinear(double channel)
{
    return channel <= 0.04045 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
}

double fromLinear(double channel)
{
    return channel <= 0.0031308 ? channel * 12.92 : 1.055 * std::pow(channel, 1 / 2.4) - 0.055;
}

Rgb linearRgb(const QColor &color)
{
    return {toLinear(color.redF()), toLinear(color.greenF()), toLinear(color.blueF())};
}

// OKLCH to linear sRGB, outside 0..1 when sRGB doesn't have the color
// (Björn Ottosson's OKLab matrices).
Rgb linearRgbFromOklch(double lightness, double chroma, double hue)
{
    const double radians = hue * std::numbers::pi / 180;
    const double a = chroma * std::cos(radians);
    const double b = chroma * std::sin(radians);
    const double l = std::pow(lightness + 0.3963377774 * a + 0.2158037573 * b, 3);
    const double m = std::pow(lightness - 0.1055613458 * a - 0.0638541728 * b, 3);
    const double s = std::pow(lightness - 0.0894841775 * a - 1.2914855480 * b, 3);
    return {4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
            -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
            -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s};
}

bool isInSrgb(const Rgb &rgb)
{
    return std::ranges::all_of(rgb, [](double channel) {
        return channel >= 0 && channel <= 1;
    });
}

// The color with that lightness and hue and the most chroma, up to
// `maximumChroma`, that sRGB has.
QColor oklchInSrgb(double lightness, double maximumChroma, double hue)
{
    Rgb rgb = linearRgbFromOklch(lightness, maximumChroma, hue);
    for (double chroma = maximumChroma; !isInSrgb(rgb) && chroma > 0; chroma -= 0.001) {
        rgb = linearRgbFromOklch(lightness, std::max(chroma, 0.0), hue);
    }
    return QColor::fromRgbF(fromLinear(rgb[0]), fromLinear(rgb[1]), fromLinear(rgb[2]));
}

double relativeLuminance(const QColor &color)
{
    const Rgb rgb = linearRgb(color);
    return 0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2];
}

}

QString enteredName(const QStringList &arguments, const QString &nameFromEnvironment)
{
    static const QStringList flagsWithoutValue = {
        QStringLiteral("-h"),  QStringLiteral("--help"),    QStringLiteral("-v"),           QStringLiteral("--verbose"),
        QStringLiteral("-T"),  QStringLiteral("-H"),        QStringLiteral("--no-tty"),     QStringLiteral("-r"),
        QStringLiteral("--root"), QStringLiteral("-V"),     QStringLiteral("--version"),    QStringLiteral("-d"),
        QStringLiteral("--dry-run"), QStringLiteral("-nw"), QStringLiteral("--no-workdir"), QStringLiteral("-Y"),
        QStringLiteral("--yes"), QStringLiteral("--clean-path"),
    };
    QString name;
    for (qsizetype i = 0; i < arguments.size(); ++i) {
        const QString &argument = arguments[i];
        if (argument == QLatin1String("-n") || argument == QLatin1String("--name")) {
            if (i + 1 < arguments.size() && !arguments[i + 1].isEmpty()) {
                name = arguments[++i];
            }
        } else if (argument == QLatin1String("-a") || argument == QLatin1String("--additional-flags")) {
            ++i;
        } else if (flagsWithoutValue.contains(argument)) {
            continue;
        } else if (argument.startsWith(QLatin1Char('-')) || argument.isEmpty()) {
            // -e, --exec and -- start the command; distrobox-enter stops at
            // anything else that starts with "-".
            break;
        } else {
            name = argument;
        }
    }
    if (name.isEmpty()) {
        name = nameFromEnvironment;
    }
    return name.isEmpty() ? QStringLiteral("my-distrobox") : name;
}

QColor konsoleTabColor(const QString &name, const QColor &accent)
{
    // Konsole's key: the detector's type and the name.
    const uint hash = qHash(QStringLiteral("distrobox:") + name);
    const int hue = static_cast<int>(hash % 360);
    const int saturation = 130 + static_cast<int>((hash >> 7) % 90);
    const int value = 170 + static_cast<int>((hash >> 15) % 60);
    const QColor hashedColor = QColor::fromHsv(hue, saturation, value);
    if (!accent.isValid()) {
        return hashedColor;
    }
    return QColor::fromRgb((hashedColor.red() * 3 + accent.red()) / 4,
                           (hashedColor.green() * 3 + accent.green()) / 4,
                           (hashedColor.blue() * 3 + accent.blue()) / 4);
}

double oklchHue(const QColor &color)
{
    const Rgb rgb = linearRgb(color);
    const double l = std::cbrt(0.4122214708 * rgb[0] + 0.5363325363 * rgb[1] + 0.0514459929 * rgb[2]);
    const double m = std::cbrt(0.2119034982 * rgb[0] + 0.6806995451 * rgb[1] + 0.1073969566 * rgb[2]);
    const double s = std::cbrt(0.0883024619 * rgb[0] + 0.2817188376 * rgb[1] + 0.6299787005 * rgb[2]);
    const double a = 1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s;
    const double b = 0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s;
    const double hue = std::atan2(b, a) * 180 / std::numbers::pi;
    return hue < 0 ? hue + 360 : hue;
}

Colors forHue(double hue)
{
    // 0.048: the most chroma sRGB has at lightness 0.28 for every hue, so
    // backgrounds look alike whatever their hue.
    return {oklchInSrgb(0.28, 0.048, hue), oklchInSrgb(0.84, 0.11, hue)};
}

double contrastRatio(const QColor &first, const QColor &second)
{
    const double firstLuminance = relativeLuminance(first);
    const double secondLuminance = relativeLuminance(second);
    return (std::max(firstLuminance, secondLuminance) + 0.05) / (std::min(firstLuminance, secondLuminance) + 0.05);
}

}
