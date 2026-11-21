#include "color.h"

#include <QTest>

using namespace WorkboxColor;

class ColorTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parse_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("color");

        QTest::newRow("#rgb") << "#3ae" << "#33aaee";
        QTest::newRow("#rrggbb") << "#3584e4" << "#3584e4";
        QTest::newRow("uppercase") << "#3584E4" << "#3584e4";
        QTest::newRow("r,g,b") << "53,132,228" << "#3584e4";
        QTest::newRow("r, g, b") << "53, 132, 228" << "#3584e4";
        QTest::newRow("limits") << "0,0,255" << "#0000ff";
        QTest::newRow("over 255") << "256,0,0" << "";
        QTest::newRow("two channels") << "53,132" << "";
        QTest::newRow("negative") << "-1,0,0" << "";
        QTest::newRow("without #") << "3584e4" << "";
        QTest::newRow("#rgba") << "#3aef" << "";
        QTest::newRow("not hexadecimal") << "#3584g4" << "";
        QTest::newRow("a name") << "red" << "";
        QTest::newRow("empty") << "" << "";
    }

    void parse()
    {
        QFETCH(QString, text);
        QFETCH(QString, color);
        const std::optional<QColor> parsed = WorkboxColor::parse(text);
        QCOMPARE(parsed ? parsed->name() : QString(), color);
    }

    void paletteHas32DistinctColors()
    {
        const QList<QColor> colors = palette();
        QCOMPARE(colors.size(), 32);
        for (const QColor &color : colors) {
            QCOMPARE(colors.count(color), 1);
        }
    }

    void distance()
    {
        QCOMPARE(WorkboxColor::distance(Qt::red, Qt::red), 0.0);
        // OKLab's lightness of black and white: 0 and 1.
        QVERIFY(qAbs(WorkboxColor::distance(Qt::black, Qt::white) - 1) < 0.001);
    }

    void noneInUsePicksTheFirst()
    {
        QCOMPARE(pickFromPalette({}), palette().first());
    }

    // For each color in use, the pick is a free one, and no other free one is
    // farther from its nearest color in use.
    void picksTheFarthestFree()
    {
        const QList<QColor> colors = palette();
        QList<QColor> colorsInUse;
        while (colorsInUse.size() < colors.size()) {
            const QColor picked = pickFromPalette(colorsInUse);
            QVERIFY(!colorsInUse.contains(picked));
            const auto distanceToNearest = [&colorsInUse](const QColor &color) {
                double nearest = INFINITY;
                for (const QColor &colorInUse : colorsInUse) {
                    nearest = std::min(nearest, WorkboxColor::distance(color, colorInUse));
                }
                return nearest;
            };
            for (const QColor &color : colors) {
                if (!colorsInUse.contains(color)) {
                    QVERIFY(distanceToNearest(color) <= distanceToNearest(picked));
                }
            }
            colorsInUse.append(picked);
        }
    }

    // The second workbox's color is across the hue circle from the first's.
    void secondIsAcrossTheHues()
    {
        const QList<QColor> colors = palette();
        const qsizetype index = colors.indexOf(pickFromPalette({colors.first()}));
        QVERIFY2(index >= 14 && index <= 18, QByteArray::number(index));
    }

    // A color given with --color counts as much as the palette's.
    void givenColorsCount()
    {
        const QColor nearFirst = palette().first().lighter(105);
        QCOMPARE(pickFromPalette({nearFirst}), pickFromPalette({palette().first()}));
    }

    void allInUsePicksTheLeastUsed()
    {
        QList<QColor> colorsInUse = palette();
        QCOMPARE(pickFromPalette(colorsInUse), palette().at(0));
        colorsInUse.append(palette().at(0));
        QCOMPARE(pickFromPalette(colorsInUse), palette().at(1));
    }
};

QTEST_GUILESS_MAIN(ColorTest)
#include "colortest.moc"
