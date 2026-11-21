// How the window shows a workbox (README.md): its color as a rounded square
// with the toolbox, and its status; and the list's rows with both.
#pragma once

#include <QStyledItemDelegate>

// The toolbox on the workbox's color, in rect.
void paintSwatch(QPainter *painter, const QRect &rect, const QColor &workboxColor);

// Running or stopped, or the operation going on (WorkboxModel's roles of
// index), translated.
QString statusText(const QModelIndex &index);

// A row of the list: the swatch, the name over the id, and the status.
class WorkboxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
