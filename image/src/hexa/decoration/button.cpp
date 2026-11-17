#include "button.h"

#include <KDecoration3/DecoratedWindow>
#include <KDecoration3/Decoration>
#include <QPainter>

using KDecoration3::DecorationButtonType;

namespace
{
// px
constexpr QSizeF BUTTON_SIZE(32, 28);
constexpr qreal BUTTON_RADIUS = 5;
constexpr qreal ICON_SIZE = 16;
// Half the side of the square each symbol fits in.
constexpr qreal SYMBOL_HALF_SIZE = 4.5;
constexpr qreal SYMBOL_STROKE_WIDTH = 1.3;
// The background's opacity over the foreground color.
constexpr qreal HOVER_OPACITY = 0.12;
constexpr qreal PRESSED_OPACITY = 0.2;
}

KDecoration3::DecorationButton *HexaButton::create(DecorationButtonType type, KDecoration3::Decoration *decoration, QObject *parent)
{
    switch (type) {
    case DecorationButtonType::Menu:
    case DecorationButtonType::Minimize:
    case DecorationButtonType::Maximize:
    case DecorationButtonType::Close: {
        auto *button = new HexaButton(type, decoration, parent);
        button->setGeometry(QRectF(QPointF(0, 0), BUTTON_SIZE));
        return button;
    }
    default:
        return nullptr;
    }
}

void HexaButton::paint(QPainter *painter, const QRectF &)
{
    if (!isVisible()) {
        return;
    }
    const auto *window = decoration()->window();
    if (type() == DecorationButtonType::Menu) {
        const QRectF iconArea(geometry().center() - QPointF(ICON_SIZE / 2, ICON_SIZE / 2), QSizeF(ICON_SIZE, ICON_SIZE));
        window->icon().paint(painter, iconArea.toRect());
        return;
    }

    const auto colorGroup = window->isActive() ? KDecoration3::ColorGroup::Active : KDecoration3::ColorGroup::Inactive;
    const QColor foreground = window->color(colorGroup, KDecoration3::ColorRole::Foreground);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    if (isHovered() || isPressed()) {
        QColor background = foreground;
        background.setAlphaF(isPressed() ? PRESSED_OPACITY : HOVER_OPACITY);
        painter->setPen(Qt::NoPen);
        painter->setBrush(background);
        painter->drawRoundedRect(geometry(), BUTTON_RADIUS, BUTTON_RADIUS);
    }
    QPen pen(foreground, SYMBOL_STROKE_WIDTH);
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    paintSymbol(painter);
    painter->restore();
}

// A line to minimize, a square to maximize, two overlapping ones to restore
// and a cross to close.
void HexaButton::paintSymbol(QPainter *painter) const
{
    const QPointF center = geometry().center();
    const QRectF symbolArea(center - QPointF(SYMBOL_HALF_SIZE, SYMBOL_HALF_SIZE), QSizeF(2 * SYMBOL_HALF_SIZE, 2 * SYMBOL_HALF_SIZE));
    switch (type()) {
    case DecorationButtonType::Minimize:
        painter->drawLine(QPointF(symbolArea.left(), center.y()), QPointF(symbolArea.right(), center.y()));
        break;
    case DecorationButtonType::Maximize:
        if (isChecked()) {
            const qreal offset = SYMBOL_HALF_SIZE / 2;
            const QRectF front = symbolArea.adjusted(0, offset, -offset, 0);
            painter->drawRect(front);
            painter->drawPolyline(QList<QPointF>{front.topLeft() + QPointF(offset, -offset), symbolArea.topRight(), front.bottomRight() + QPointF(offset, -offset)});
        } else {
            painter->drawRect(symbolArea);
        }
        break;
    case DecorationButtonType::Close:
        painter->drawLine(symbolArea.topLeft(), symbolArea.bottomRight());
        painter->drawLine(symbolArea.topRight(), symbolArea.bottomLeft());
        break;
    default:
        break;
    }
}
