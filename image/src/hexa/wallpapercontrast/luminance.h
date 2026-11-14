#pragma once

#include <QImage>
#include <QRect>

// Whether white reads better than black on $area of $image, by the WCAG
// contrast of each with the area's mean luminance.
bool isDarkArea(const QImage &image, const QRect &area);
