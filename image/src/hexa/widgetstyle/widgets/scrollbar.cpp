#include "scrollbar.h"

#include <QPainter>
#include <QProxyStyle>
#include <QStyleOptionComplex>

namespace
{
// The width of Breeze's line (PenWidth::Frame in breezemetrics.h, as the
// QRect Breeze draws holds it).
constexpr int LINE_WIDTH = 1;

// Where Breeze draws the line (Style::drawScrollBarComplexControl in
// breezestyle.cpp): inside the scroll bar, along the side next to the
// content; the rest of the scroll bar leaves it free.
QRect breezeLine(const QStyleOptionComplex *option)
{
    if (option->state & QStyle::State_Horizontal) {
        return QRect(0, 0, option->rect.width(), LINE_WIDTH);
    }
    return QStyle::alignedRect(option->direction, Qt::AlignLeft, QSize(LINE_WIDTH, option->rect.height()), option->rect);
}
}

void drawScrollBar(const QProxyStyle &style, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget)
{
    // Breeze draws it all, but nothing reaches the line's strip.
    painter->save();
    painter->setClipRegion(QRegion(option->rect).subtracted(breezeLine(option)), Qt::IntersectClip);
    // QProxyStyle's, not the style's own: that one calls this.
    style.QProxyStyle::drawComplexControl(QStyle::CC_ScrollBar, option, painter, widget);
    painter->restore();
}
