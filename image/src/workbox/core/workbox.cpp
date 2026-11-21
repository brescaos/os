#include "workbox.h"

#include <QRegularExpression>

namespace
{
const QString colorLabel = QStringLiteral("com.brescaos.workbox.color");
// Only on the workboxes with sudo.
const QString sudoLabel = QStringLiteral("com.brescaos.workbox.sudo");
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
    QMap<QString, QString> labels{{nameLabel, workbox.name}, {colorLabel, workbox.color.name()}};
    if (workbox.hasSudo) {
        labels.insert(sudoLabel, QStringLiteral("true"));
    }
    return labels;
}

Workbox workboxFromLabels(const QString &id, const QMap<QString, QString> &labels)
{
    return {.id = id,
            .name = labels.value(nameLabel),
            .color = QColor(labels.value(colorLabel)),
            .hasSudo = labels.value(sudoLabel) == QLatin1String("true")};
}
