#include "terminalcolors.h"

#include "color.h"

TerminalColors terminalColorsFor(const QColor &workboxColor)
{
    const double hue = WorkboxColor::oklchHue(workboxColor);
    // OKLCH lightness 0.28 with chroma 0.048, which sRGB has for every hue,
    // so every background looks alike but for its hue; the label at 0.84, with
    // as much chroma as sRGB has up to 0.11.
    return {.background = WorkboxColor::fromOklch(0.28, 0.048, hue),
            .text = QColor(0xfc, 0xfc, 0xfc),
            .label = WorkboxColor::fromOklch(0.84, 0.11, hue)};
}
