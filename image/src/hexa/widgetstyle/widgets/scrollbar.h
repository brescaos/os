// Scroll bars: Breeze's, without the line Breeze draws along them.

#pragma once

class QPainter;
class QProxyStyle;
class QStyleOptionComplex;
class QWidget;

// Draws the scroll bar with $style's base style, Breeze, leaving out the line.
void drawScrollBar(const QProxyStyle &style, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget);
