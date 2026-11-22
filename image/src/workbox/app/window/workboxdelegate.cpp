#include "workboxdelegate.h"

#include "workboxmodel.h"

#include <KColorScheme>
#include <KLocalizedString>

#include <QApplication>
#include <QFontDatabase>
#include <QPainter>
#include <QPainterPath>

namespace
{

constexpr int swatchSize = 32;
constexpr int rowHeight = 56;
constexpr int padding = 12;
constexpr int statusDotSize = 8;

}

void paintSwatch(QPainter *painter, const QRect &rect, const QColor &workboxColor)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QPainterPath swatch;
    swatch.addRoundedRect(rect, rect.width() * 0.22, rect.height() * 0.22);
    painter->fillPath(swatch, workboxColor);
    // The toolbox draws in its fallback color, dark on every workbox color.
    const int iconSize = rect.width() * 9 / 16;
    QIcon::fromTheme(QStringLiteral("com.brescaos.workbox-symbolic")).paint(painter, QRect(QPoint(), QSize(iconSize, iconSize)).translated(rect.center() - QPoint(iconSize / 2, iconSize / 2)));
    painter->restore();
}

QPixmap swatchPixmap(const QColor &workboxColor, int size, qreal pixelRatio)
{
    QPixmap swatch(QSize(size, size) * pixelRatio);
    swatch.setDevicePixelRatio(pixelRatio);
    swatch.fill(Qt::transparent);
    QPainter painter(&swatch);
    paintSwatch(&painter, QRect(0, 0, size, size), workboxColor);
    return swatch;
}

QString statusText(const QModelIndex &index)
{
    switch (static_cast<WorkboxModel::Operation>(index.data(WorkboxModel::OperationRole).toInt())) {
    case WorkboxModel::Operation::Starting:
        return i18nc("@info:status a workbox", "Starting…");
    case WorkboxModel::Operation::Stopping:
        return i18nc("@info:status a workbox", "Stopping…");
    case WorkboxModel::Operation::Removing:
        return i18nc("@info:status a workbox", "Removing…");
    case WorkboxModel::Operation::None:
        break;
    }
    return index.data(WorkboxModel::RunningRole).toBool() ? i18nc("@info:status a workbox", "Running") : i18nc("@info:status a workbox", "Stopped");
}

void WorkboxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem background = option;
    initStyleOption(&background, index);
    background.text.clear();
    background.icon = QIcon();
    QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &background, painter, option.widget);

    const QRect content = option.rect.adjusted(padding, 0, -padding, 0);
    const QRect swatch(content.left(), content.center().y() - swatchSize / 2, swatchSize, swatchSize);
    paintSwatch(painter, swatch, index.data(WorkboxModel::ColorRole).value<QColor>());

    const bool isSelected = option.state & QStyle::State_Selected;
    const QPalette::ColorGroup group = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    const QColor textColor = option.palette.color(group, isSelected ? QPalette::HighlightedText : QPalette::Text);
    const KColorScheme colors(group, KColorScheme::View);
    const bool isRunning = index.data(WorkboxModel::RunningRole).toBool();

    // The status on the right, with its dot.
    const QString status = statusText(index);
    QFont statusFont = option.font;
    statusFont.setPointSizeF(option.font.pointSizeF() * 0.9);
    const int statusWidth = QFontMetrics(statusFont).horizontalAdvance(status);
    const QRect statusRect(content.right() - statusWidth, content.top(), statusWidth, content.height());
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->setBrush(isRunning ? colors.foreground(KColorScheme::PositiveText).color() : colors.foreground(KColorScheme::InactiveText).color());
    painter->drawEllipse(QRect(statusRect.left() - statusDotSize - 6, content.center().y() - statusDotSize / 2, statusDotSize, statusDotSize));
    painter->setFont(statusFont);
    painter->setPen(isSelected ? textColor : colors.foreground(KColorScheme::InactiveText).color());
    painter->drawText(statusRect, Qt::AlignVCenter | Qt::AlignRight, status);

    // The name over the id.
    const int textLeft = swatch.right() + padding;
    const int textWidth = statusRect.left() - statusDotSize - 2 * padding - textLeft;
    QFont nameFont = option.font;
    nameFont.setWeight(QFont::DemiBold);
    QFont idFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    idFont.setPointSizeF(option.font.pointSizeF() * 0.85);
    const QFontMetrics nameMetrics(nameFont);
    const QFontMetrics idMetrics(idFont);
    const int top = content.center().y() - (nameMetrics.height() + idMetrics.height()) / 2;
    painter->setFont(nameFont);
    painter->setPen(textColor);
    painter->drawText(QRect(textLeft, top, textWidth, nameMetrics.height()),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      nameMetrics.elidedText(index.data(WorkboxModel::NameRole).toString(), Qt::ElideRight, textWidth));
    painter->setFont(idFont);
    painter->setPen(isSelected ? textColor : colors.foreground(KColorScheme::InactiveText).color());
    painter->drawText(QRect(textLeft, top + nameMetrics.height(), textWidth, idMetrics.height()),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      idMetrics.elidedText(index.data(WorkboxModel::IdRole).toString(), Qt::ElideRight, textWidth));
    painter->restore();
}

QSize WorkboxDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(QStyledItemDelegate::sizeHint(option, index).width(), rowHeight);
}
