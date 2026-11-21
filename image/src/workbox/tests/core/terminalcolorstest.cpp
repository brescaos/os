#include "color.h"
#include "terminalcolors.h"

#include <QTest>

#include <cmath>

namespace
{

// WCAG 2's contrast ratio, from 1 to 21.
double contrastRatio(const QColor &first, const QColor &second)
{
    const auto luminance = [](const QColor &color) {
        const auto linear = [](double channel) {
            return channel <= 0.04045 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
        };
        return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
    };
    const double lighter = std::max(luminance(first), luminance(second));
    const double darker = std::min(luminance(first), luminance(second));
    return (lighter + 0.05) / (darker + 0.05);
}

}

class TerminalColorsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void oklchHueOfPrimaries()
    {
        QCOMPARE(qRound(WorkboxColor::oklchHue(QColor(255, 0, 0))), 29);
        QCOMPARE(qRound(WorkboxColor::oklchHue(QColor(0, 255, 0))), 142);
        QCOMPARE(qRound(WorkboxColor::oklchHue(QColor(0, 0, 255))), 264);
    }

    // Every hue: AAA (7:1) for the text and the label on the background, and
    // both keep the workbox's hue.
    void forEveryHue()
    {
        for (double hue = 0; hue < 360; hue += 0.5) {
            const TerminalColors colors = terminalColorsFor(WorkboxColor::fromOklch(0.7, 0.11, hue));
            const QByteArray message = QByteArray::number(hue);
            QVERIFY2(contrastRatio(colors.text, colors.background) >= 7, message);
            QVERIFY2(contrastRatio(colors.label, colors.background) >= 7, message);
            for (const QColor &color : {colors.background, colors.label}) {
                const double difference = std::abs(WorkboxColor::oklchHue(color) - hue);
                QVERIFY2(std::min(difference, 360 - difference) < 3, message);
            }
        }
    }

    // A color given with --color, however dark or light, gives the same
    // colors as the palette's of its hue.
    void onlyTheHueCounts()
    {
        const TerminalColors dark = terminalColorsFor(QColor(40, 0, 0));
        const TerminalColors light = terminalColorsFor(QColor(255, 200, 200));
        QVERIFY(WorkboxColor::distance(dark.background, light.background) < 0.02);
        QVERIFY(contrastRatio(dark.label, dark.background) >= 7);
    }
};

QTEST_GUILESS_MAIN(TerminalColorsTest)
#include "terminalcolorstest.moc"
