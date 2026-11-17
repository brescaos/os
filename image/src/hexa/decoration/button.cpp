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
// Half the side of the square each symbol fits in. With the button's center
// on a pixel corner, its sides are on pixel edges, where 2 px strokes stay
// sharp; a 1.3 px one, or one off the edges, blurs into its neighbors.
constexpr qreal SYMBOL_HALF_SIZE = 4;
constexpr qreal SYMBOL_STROKE_WIDTH = 2;
// The restore symbol's back square, up and right of the front one, whole px.
constexpr qreal RESTORE_OFFSET = 3;
// The close cross: a diagonal looks heavier than a straight line of the same
// width.
constexpr qreal CLOSE_STROKE_WIDTH = 1.8;
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
    // Ends and corners as sharp as the sides.
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::MiterJoin);
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
    case DecorationButtonType::Minimize: {
        // As wide as the square, its stroke included.
        const qreal halfLength = SYMBOL_HALF_SIZE + SYMBOL_STROKE_WIDTH / 2;
        painter->drawLine(QPointF(center.x() - halfLength, center.y()), QPointF(center.x() + halfLength, center.y()));
        break;
    }
    case DecorationButtonType::Maximize:
        if (isChecked()) {
            const QRectF front = symbolArea.adjusted(0, RESTORE_OFFSET, -RESTORE_OFFSET, 0);
            painter->drawRect(front);
            painter->drawPolyline(QList<QPointF>{front.topLeft() + QPointF(RESTORE_OFFSET, -RESTORE_OFFSET), symbolArea.topRight(),
                                                 front.bottomRight() + QPointF(RESTORE_OFFSET, -RESTORE_OFFSET)});
        } else {
            painter->drawRect(symbolArea);
        }
        break;
    case DecorationButtonType::Close: {
        QPen pen = painter->pen();
        pen.setWidthF(CLOSE_STROKE_WIDTH);
        pen.setCapStyle(Qt::RoundCap);
        painter->setPen(pen);
        painter->drawLine(symbolArea.topLeft(), symbolArea.bottomRight());
        painter->drawLine(symbolArea.topRight(), symbolArea.bottomLeft());
        break;
    }
    default:
        break;
    }
}
