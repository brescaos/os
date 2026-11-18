// Hexa's QtWidgets style (README.md): Breeze's, without the line Breeze
// draws along every scroll bar. Qt Quick apps get it too: KDE's desktop
// style draws their scroll bars with the QtWidgets style.

#pragma once

#include <QProxyStyle>

class HexaStyle : public QProxyStyle
{
    Q_OBJECT

public:
    // On Breeze, or on Qt's default style if Breeze isn't installed.
    HexaStyle();

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter,
                            const QWidget *widget = nullptr) const override;
};
