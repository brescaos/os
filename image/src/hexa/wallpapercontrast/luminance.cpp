#include "luminance.h"

#include <cmath>

// The luminance white and black contrast equally with:
// 1.05 / (L + 0.05) = (L + 0.05) / 0.05.
static const double EqualContrastLuminance = std::sqrt(1.05 * 0.05) - 0.05;

// An sRGB channel (0-255) in linear light (0-1), as WCAG's relative luminance
// takes it.
static double linearChannel(int channel)
{
    const double value = channel / 255.0;
    return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
}

bool isDarkArea(const QImage &image, const QRect &area)
{
    const QImage pixels = image.copy(area).convertToFormat(QImage::Format_RGB32);
    double luminanceSum = 0;
    for (int y = 0; y < pixels.height(); ++y) {
        const QRgb *row = reinterpret_cast<const QRgb *>(pixels.constScanLine(y));
        for (int x = 0; x < pixels.width(); ++x) {
            luminanceSum += 0.2126 * linearChannel(qRed(row[x])) + 0.7152 * linearChannel(qGreen(row[x]))
                + 0.0722 * linearChannel(qBlue(row[x]));
        }
    }
    return luminanceSum / (pixels.width() * pixels.height()) < EqualContrastLuminance;
}
