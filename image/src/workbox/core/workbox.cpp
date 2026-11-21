#include "workbox.h"

#include <QRegularExpression>

namespace
{
const QString colorLabel = QStringLiteral("com.brescaos.workbox.color");
}

bool isValidId(const QString &id)
{
    static const QRegularExpression validId(QStringLiteral("^[a-z0-9][a-z0-9-]{0,31}$"));
    return validId.match(id).hasMatch();
}

QString homeVolumeName(const QString &id)
{
    return QStringLiteral("workbox-%1-home").arg(id);
}

QMap<QString, QString> labelsOf(const Workbox &workbox)
{
    return {{nameLabel, workbox.name}, {colorLabel, workbox.color.name()}};
}

Workbox workboxFromLabels(const QString &id, const QMap<QString, QString> &labels)
{
    return {.id = id, .name = labels.value(nameLabel), .color = QColor(labels.value(colorLabel))};
}
