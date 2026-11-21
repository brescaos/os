#include "workbox.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace
{
const QString colorLabel = QStringLiteral("com.brescaos.workbox.color");
// Only on the workboxes with sudo, with Wayland.
const QString sudoLabel = QStringLiteral("com.brescaos.workbox.sudo");
const QString waylandLabel = QStringLiteral("com.brescaos.workbox.wayland");
// Only on the workboxes with volumes: JSON,
// [{"source": ..., "name": ..., "target": ..., "readOnly": ...}].
const QString volumesLabel = QStringLiteral("com.brescaos.workbox.volumes");
}

bool isValidId(const QString &id)
{
    static const QRegularExpression validId(QStringLiteral("^[a-z0-9][a-z0-9-]{0,31}$"));
    return validId.match(id).hasMatch();
}

QString volumeName(const QString &id, const QString &name)
{
    return QStringLiteral("workbox-%1.%2").arg(id, name);
}

bool isValidVolume(const QString &name)
{
    static const QRegularExpression validVolume(QStringLiteral("^[a-z0-9][a-z0-9-]{0,31}$"));
    return name != homeVolume && validVolume.match(name).hasMatch();
}

QMap<QString, QString> labelsOf(const Workbox &workbox)
{
    QMap<QString, QString> labels{{nameLabel, workbox.name}, {colorLabel, workbox.color.name()}};
    if (workbox.hasSudo) {
        labels.insert(sudoLabel, QStringLiteral("true"));
    }
    if (workbox.hasWayland) {
        labels.insert(waylandLabel, QStringLiteral("true"));
    }
    if (!workbox.volumes.isEmpty()) {
        QJsonArray volumes;
        for (const Volume &volume : workbox.volumes) {
            volumes.append(QJsonObject{{QStringLiteral("source"), volume.source},
                                      {QStringLiteral("name"), volume.name},
                                      {QStringLiteral("target"), volume.target},
                                      {QStringLiteral("readOnly"), volume.isReadOnly}});
        }
        labels.insert(volumesLabel, QString::fromUtf8(QJsonDocument(volumes).toJson(QJsonDocument::Compact)));
    }
    return labels;
}

Workbox workboxFromLabels(const QString &id, const QMap<QString, QString> &labels)
{
    QList<Volume> volumes;
    for (const QJsonValue &volume : QJsonDocument::fromJson(labels.value(volumesLabel).toUtf8()).array()) {
        volumes.append({.source = volume[QLatin1String("source")].toString(),
                       .name = volume[QLatin1String("name")].toString(),
                       .target = volume[QLatin1String("target")].toString(),
                       .isReadOnly = volume[QLatin1String("readOnly")].toBool()});
    }
    return {.id = id,
            .name = labels.value(nameLabel),
            .color = QColor(labels.value(colorLabel)),
            .hasSudo = labels.value(sudoLabel) == QLatin1String("true"),
            .hasWayland = labels.value(waylandLabel) == QLatin1String("true"),
            .volumes = volumes};
}
