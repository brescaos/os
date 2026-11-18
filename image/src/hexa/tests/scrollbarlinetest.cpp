// Hexa's QtWidgets style draws Breeze's scroll bars without the line along
// them (widgetstyle/hexastyle.h). It needs Breeze's style installed: without
// it, there's no line to leave out.
#include "hexastyle.h"

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QStyleFactory>
#include <QStyleOptionSlider>
#include <QTest>

#include <memory>

namespace
{
constexpr QSize SCROLL_BAR_SIZE(16, 200);

QImage drawScrollBar(QStyle *style, Qt::Orientation orientation, Qt::LayoutDirection direction)
{
    QStyleOptionSlider option;
    option.orientation = orientation;
    option.direction = direction;
    option.rect = QRect(QPoint(0, 0), orientation == Qt::Vertical ? SCROLL_BAR_SIZE : SCROLL_BAR_SIZE.transposed());
    option.state = QStyle::State_Enabled;
    if (orientation == Qt::Horizontal) {
        option.state |= QStyle::State_Horizontal;
    }
    option.minimum = 0;
    option.maximum = 100;
    option.pageStep = 20;
    option.subControls = QStyle::SC_All;

    QImage image(option.rect.size(), QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    style->drawComplexControl(QStyle::CC_ScrollBar, &option, &painter);
    return image;
}

// Whether every pixel of $area is white.
bool isWhite(const QImage &image, const QRect &area)
{
    for (int y = area.top(); y <= area.bottom(); ++y) {
        for (int x = area.left(); x <= area.right(); ++x) {
            if (image.pixel(x, y) != qRgb(255, 255, 255)) {
                return false;
            }
        }
    }
    return true;
}
}

class ScrollBarLineTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        if (!QStyleFactory::keys().contains(QStringLiteral("Breeze"), Qt::CaseInsensitive)) {
            QSKIP("Breeze's style isn't installed");
        }
    }

    void noLine_data()
    {
        QTest::addColumn<Qt::Orientation>("orientation");
        QTest::addColumn<Qt::LayoutDirection>("direction");
        // Breeze's line: the side next to the content.
        QTest::addColumn<QRect>("line");

        const int length = SCROLL_BAR_SIZE.height();
        const int right = SCROLL_BAR_SIZE.width() - 1;
        QTest::newRow("vertical") << Qt::Vertical << Qt::LeftToRight << QRect(0, 0, 1, length);
        QTest::newRow("vertical, right to left") << Qt::Vertical << Qt::RightToLeft << QRect(right, 0, 1, length);
        QTest::newRow("horizontal") << Qt::Horizontal << Qt::LeftToRight << QRect(0, 0, length, 1);
    }

    void noLine()
    {
        QFETCH(Qt::Orientation, orientation);
        QFETCH(Qt::LayoutDirection, direction);
        QFETCH(QRect, line);

        // Breeze draws it there, so the test looks where it should.
        std::unique_ptr<QStyle> breeze(QStyleFactory::create(QStringLiteral("Breeze")));
        QVERIFY(!isWhite(drawScrollBar(breeze.get(), orientation, direction), line));

        HexaStyle hexa;
        QVERIFY(isWhite(drawScrollBar(&hexa, orientation, direction), line));
    }
};

QTEST_MAIN(ScrollBarLineTest)
#include "scrollbarlinetest.moc"
