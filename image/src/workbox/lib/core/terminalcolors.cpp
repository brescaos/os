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

QString colorsSequence(const TerminalColors &colors)
{
    return QStringLiteral("\x1b]11;%1\x07\x1b]10;%2\x07").arg(colors.background.name(), colors.text.name());
}

QMap<QString, QString> promptEnvironment(const QString &workboxName, const TerminalColors &colors)
{
    // The name goes in its own variable: bash expands it in the prompt
    // without reading what's in it as escapes or commands.
    const QString prompt = QStringLiteral("\\[\\e[1;38;2;%1;%2;%3m\\]\U0001F9F0 ${WORKBOX_NAME}\\[\\e[0m\\] [\\u@\\h \\W]\\$ ")
                               .arg(colors.label.red())
                               .arg(colors.label.green())
                               .arg(colors.label.blue());
    return {{QStringLiteral("PS1"), prompt}, {QStringLiteral("WORKBOX_NAME"), workboxName}};
}
