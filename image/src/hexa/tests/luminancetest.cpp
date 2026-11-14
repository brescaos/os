// Whether an area of the wallpaper is dark (wallpapercontrast/luminance.h).
#include "luminance.h"

#include <QTest>

class LuminanceTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void plainColors_data()
    {
        QTest::addColumn<QColor>("color");
        QTest::addColumn<bool>("isDark");

        QTest::newRow("black") << QColor(Qt::black) << true;
        QTest::newRow("white") << QColor(Qt::white) << false;
        QTest::newRow("navy") << QColor("#1b2a49") << true;
        QTest::newRow("sky blue") << QColor("#87ceeb") << false;
        // Hexa's accent: black reads better on it.
        QTest::newRow("yellow") << QColor("#f2b33d") << false;
        // Either side of the luminance white and black contrast equally with
        // (0.179: sRGB gray 117.6).
        QTest::newRow("gray below the middle") << QColor(112, 112, 112) << true;
        QTest::newRow("gray above the middle") << QColor(124, 124, 124) << false;
    }

    void plainColors()
    {
        QFETCH(QColor, color);
        QFETCH(bool, isDark);

        QImage image(16, 16, QImage::Format_RGB32);
        image.fill(color);

        QCOMPARE(isDarkArea(image, image.rect()), isDark);
    }

    // A light sky over dark ground: only the area asked for counts.
    void onlyTheAreaCounts()
    {
        QImage image(16, 16, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::black);
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < image.width(); ++x) {
                image.setPixelColor(x, y, Qt::white);
            }
        }

        QVERIFY(!isDarkArea(image, QRect(0, 0, 16, 4)));
        QVERIFY(isDarkArea(image, QRect(0, 4, 16, 12)));
        // Mostly black, but its mean is past the middle: luminance isn't
        // linear in what the eye sees.
        QVERIFY(!isDarkArea(image, image.rect()));
    }
};

QTEST_GUILESS_MAIN(LuminanceTest)

#include "luminancetest.moc"
