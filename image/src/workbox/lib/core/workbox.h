// A workbox in workbox's own terms (README.md), the same with any engine: its
// id, name and color, the labels that keep them and its home volume's name,
// and what only the engine knows: whether it's running, and its image.
#pragma once

#include <QColor>
#include <QMap>
#include <QString>

struct Workbox {
    QString id;
    QString name;
    QColor color;
    // The user can use sudo in it, without a password.
    bool hasSudo = false;
    bool isRunning = false;
    QString image;
};

// What every command fails with: its message is for the user, translated.
struct WorkboxError {
    QString message;
};

// Every workbox's container has it, so it's how the engines find them.
inline const QString nameLabel = QStringLiteral("com.brescaos.workbox.name");

// [a-z0-9][a-z0-9-]*, up to 32 characters.
bool isValidId(const QString &id);

QString homeVolumeName(const QString &id);

QMap<QString, QString> labelsOf(const Workbox &workbox);

Workbox workboxFromLabels(const QString &id, const QMap<QString, QString> &labels);
