// The title bar's buttons: the app's icon (the window menu), minimize,
// maximize and close.

#pragma once

#include <KDecoration3/DecorationButton>

class HexaButton : public KDecoration3::DecorationButton
{
public:
    using DecorationButton::DecorationButton;

    // Only the buttons Hexa draws: the group leaves out the rest.
    static KDecoration3::DecorationButton *create(KDecoration3::DecorationButtonType type, KDecoration3::Decoration *decoration, QObject *parent);

    void paint(QPainter *painter, const QRectF &repaintArea) override;

private:
    void paintSymbol(QPainter *painter) const;
};
