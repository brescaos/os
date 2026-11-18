#include "hexastyle.h"

#include "widgets/scrollbar.h"

#include <QStyleFactory>

HexaStyle::HexaStyle()
    : QProxyStyle(QStyleFactory::create(QStringLiteral("Breeze")))
{
}

void HexaStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter,
                                   const QWidget *widget) const
{
    if (control == CC_ScrollBar && option) {
        drawScrollBar(*this, option, painter, widget);
        return;
    }
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}
