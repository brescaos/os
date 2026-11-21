#include "volumes.h"

#include <KLocalizedString>

#include <QDir>
#include <QFileInfo>

namespace
{

// Relative to home. Keys, and what the host runs code from: a workbox that
// could write there would run code on the host at the next login.
const QStringList blockedPaths{
    QStringLiteral(".ssh"),
    QStringLiteral(".gnupg"),
    QStringLiteral(".config/autostart"),
    QStringLiteral(".config/systemd"),
    QStringLiteral(".config/environment.d"),
    QStringLiteral(".config/plasma-workspace"),
    QStringLiteral(".local/bin"),
    QStringLiteral(".local/share/applications"),
    QStringLiteral(".local/share/kservices5"),
    QStringLiteral(".local/share/kservices6"),
    QStringLiteral(".local/share/plasma"),
};

// Other apps' settings and data: only read-only, and never whole.
const QStringList readOnlyPaths{QStringLiteral(".config"), QStringLiteral(".local")};

bool isInside(const QString &path, const QString &folder)
{
    return path.startsWith(folder + QLatin1Char('/'));
}

// given, a leading ~ for home, as an absolute path in home: never home itself,
// whose volume it would hide in the workbox, nor anything outside it.
QString targetPath(const QString &given, const QString &home)
{
    QString target = given == QLatin1String("~") || given.startsWith(QLatin1String("~/")) ? home + given.mid(1) : given;
    if (target.size() > 1 && target.endsWith(QLatin1Char('/'))) {
        target.chop(1);
    }
    if (!QDir::isAbsolutePath(target) || QDir::cleanPath(target) != target) {
        throw WorkboxError{i18n("can't share at %1: give an absolute path, without “..”", given)};
    }
    if (target.contains(QLatin1Char(':')) || target.contains(QLatin1Char(','))) {
        throw WorkboxError{i18n("can't share at %1: its path has a “:” or a “,”", target)};
    }
    if (!isInside(target, home)) {
        throw WorkboxError{i18n("can't share at %1: it must be a folder in the home folder, not the home folder itself", target)};
    }
    return target;
}

// The real path of folder, a folder of home, or a WorkboxError saying why it
// can't be other.
QString sourcePath(const QString &folder, const QString &home, bool isReadOnly)
{
    const QFileInfo info(QDir::current(), folder);
    if (!info.isDir()) {
        throw WorkboxError{i18n("can't share %1: it isn't a folder", folder)};
    }
    const QString path = info.canonicalFilePath();
    // Podman's --volume separates with them.
    if (path.contains(QLatin1Char(':')) || path.contains(QLatin1Char(','))) {
        throw WorkboxError{i18n("can't share %1: its path has a “:” or a “,”", path)};
    }
    if (path == home) {
        throw WorkboxError{i18n("can't share %1: it's the whole home folder", path)};
    }
    if (!isInside(path, home)) {
        throw WorkboxError{i18n("can't share %1: it's outside the home folder", path)};
    }
    for (const QString &readOnly : readOnlyPaths) {
        if (path == home + QLatin1Char('/') + readOnly) {
            throw WorkboxError{i18n("can't share %1 whole: it holds other apps' settings; share one of its folders, with :ro", path)};
        }
    }
    for (const QString &blocked : blockedPaths) {
        const QString blockedPath = home + QLatin1Char('/') + blocked;
        if (path == blockedPath || isInside(path, blockedPath)) {
            throw WorkboxError{i18n("can't share %1: it holds keys or what the host runs at login", path)};
        }
        if (isInside(blockedPath, path)) {
            throw WorkboxError{i18n("can't share %1: it holds %2, with keys or what the host runs at login", path, blockedPath)};
        }
    }
    for (const QString &readOnly : readOnlyPaths) {
        if (isInside(path, home + QLatin1Char('/') + readOnly) && !isReadOnly) {
            throw WorkboxError{i18n("can't share %1 to write in it: it holds an app's settings; add :ro", path)};
        }
    }
    return path;
}

// Whether options, Podman's, make it read-only; only those workbox allows.
bool isReadOnlyFrom(const QString &options)
{
    bool isReadOnly = false;
    bool isReadWrite = false;
    for (const QString &option : options.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        if (option == QLatin1String("ro")) {
            isReadOnly = true;
        } else if (option == QLatin1String("rw")) {
            isReadWrite = true;
        } else if (option == QLatin1String("Z")) {
            throw WorkboxError{i18n("the volume option Z isn't allowed: it would take the folder from other workboxes that share it")};
        } else if (option == QLatin1String("U")) {
            throw WorkboxError{i18n("the volume option U isn't allowed: it would change the owner of the host's files")};
        } else if (option != QLatin1String("z")) {
            // z: workbox always mounts with it.
            throw WorkboxError{i18n("the volume option %1 isn't supported: only ro and rw", option)};
        }
    }
    if (isReadOnly && isReadWrite) {
        throw WorkboxError{i18n("a volume can't be both ro and rw")};
    }
    return isReadOnly;
}

bool isPath(const QString &text)
{
    return text.startsWith(QLatin1Char('/')) || text.startsWith(QLatin1Char('~')) || text.startsWith(QLatin1Char('.'));
}

}

Volume parseVolume(const QString &argument, const QString &home)
{
    // Podman's forms: target, source:target, target:options and
    // source:target:options; the second part is a target if it's a path.
    const QStringList parts = argument.split(QLatin1Char(':'));
    QString source;
    QString target;
    QString options;
    if (parts.size() == 1) {
        target = parts[0];
    } else if (parts.size() == 2 && !isPath(parts[1])) {
        target = parts[0];
        options = parts[1];
    } else if (parts.size() <= 3) {
        source = parts[0];
        target = parts[1];
        options = parts.value(2);
    } else {
        throw WorkboxError{i18n("invalid volume %1: use [folder|name:]target[:options]", argument)};
    }

    const QString realHome = QFileInfo(home).canonicalFilePath();
    const bool isReadOnly = isReadOnlyFrom(options);
    if (parts.size() > 1 && source.isEmpty() && options.isEmpty()) {
        throw WorkboxError{i18n("invalid volume %1: use [folder|name:]target[:options]", argument)};
    }
    // A name, as Podman's named volume: one of the workbox's own.
    if (!source.isEmpty() && !isPath(source) && !source.contains(QLatin1Char('/'))) {
        if (!isValidVolume(source)) {
            throw WorkboxError{i18n("invalid volume name %1: up to 32 lowercase letters (a-z), digits and hyphens, not starting with a hyphen, and not home", source)};
        }
        return {.name = source, .target = targetPath(target, realHome), .isReadOnly = isReadOnly};
    }
    const QString expandedSource = source == QLatin1String("~") || source.startsWith(QLatin1String("~/")) ? realHome + source.mid(1) : source;
    return {.source = source.isEmpty() ? QString() : sourcePath(expandedSource, realHome, isReadOnly),
            .target = targetPath(target, realHome),
            .isReadOnly = isReadOnly};
}

QString volumeDescription(const Volume &volume)
{
    const QString folders = !volume.name.isEmpty()
        ? i18nc("a volume of the workbox's own in workbox info", "%1 → %2 (volume)", volume.name, volume.target)
        : volume.source.isEmpty()        ? i18nc("an unnamed volume of the workbox's own in workbox info", "%1 (its own)", volume.target)
        : volume.target == volume.source ? volume.source
                                         : volume.source + QStringLiteral(" → ") + volume.target;
    return volume.isReadOnly ? i18nc("a volume in workbox info", "%1 (read-only)", folders) : folders;
}
