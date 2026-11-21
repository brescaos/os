// A workbox in workbox's own terms (README.md), the same with any engine: its
// id, name and color, the labels that keep them and its volumes' names,
// and what only the engine knows: whether it's running, and its image.
#pragma once

#include <QColor>
#include <QList>
#include <QMap>
#include <QString>

// What the workbox sees at target, in its home (volumes.h): a host folder
// (source); a volume of its own, named (name), or without a name (neither):
// both are the workbox's, and removed with it.
struct Volume {
    QString source;
    QString name;
    QString target;
    bool isReadOnly = false;

    bool operator==(const Volume &) const = default;
};

struct Workbox {
    QString id;
    QString name;
    QColor color;
    // The user can use sudo in it, without a password.
    bool hasSudo = false;
    // Its apps can show windows in the user's Wayland session, with the GPU.
    bool hasWayland = false;
    QList<Volume> volumes;
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

// Every volume of the workbox id: workbox-<id>.<name>. Ids have no dots, so
// two workboxes' volumes never have the same name.
QString volumeName(const QString &id, const QString &name);

// Its home: the volume named home.
inline const QString homeVolume = QStringLiteral("home");

// [a-z0-9][a-z0-9-]*, up to 32 characters, and not home.
bool isValidVolume(const QString &name);

QMap<QString, QString> labelsOf(const Workbox &workbox);

Workbox workboxFromLabels(const QString &id, const QMap<QString, QString> &labels);
