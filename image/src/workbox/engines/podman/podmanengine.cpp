#include "podmanengine.h"

#include <KLocalizedString>

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

namespace
{

// Its standard output; podman's own errors go straight to stderr.
QString runPodman(const QStringList &arguments)
{
    QProcess podman;
    podman.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    podman.start(QStringLiteral("podman"), arguments);
    if (!podman.waitForFinished(-1) && podman.error() == QProcess::FailedToStart) {
        throw WorkboxError{i18n("couldn't run podman: %1", podman.errorString())};
    }
    if (podman.exitStatus() != QProcess::NormalExit || podman.exitCode() != 0) {
        throw WorkboxError{i18n("podman %1 failed", arguments.join(QLatin1Char(' ')))};
    }
    return QString::fromUtf8(podman.readAllStandardOutput());
}

}

QList<Workbox> PodmanEngine::list()
{
    const QString json = runPodman({QStringLiteral("ps"),
                                    QStringLiteral("--all"),
                                    QStringLiteral("--filter"),
                                    QStringLiteral("label=") + nameLabel,
                                    QStringLiteral("--format"),
                                    QStringLiteral("json")});
    QList<Workbox> workboxes;
    for (const QJsonValue &container : QJsonDocument::fromJson(json.toUtf8()).array()) {
        const QJsonObject object = container.toObject();
        QMap<QString, QString> labels;
        const QJsonObject labelObject = object[QLatin1String("Labels")].toObject();
        for (auto label = labelObject.begin(); label != labelObject.end(); ++label) {
            labels.insert(label.key(), label.value().toString());
        }
        workboxes.append(workboxFromLabels(object[QLatin1String("Names")].toArray().first().toString(), labels));
    }
    return workboxes;
}

void PodmanEngine::create(const Workbox &workbox, const QString &image)
{
    // The same uid inside, so the files in shared folders keep their owner;
    // the home volume at the same path as the user's home, which keep-id
    // gives the user inside too. Podman creates the volume.
    QStringList arguments{QStringLiteral("create"),
                          QStringLiteral("--name"),
                          workbox.id,
                          QStringLiteral("--hostname"),
                          workbox.id,
                          QStringLiteral("--userns=keep-id"),
                          QStringLiteral("--network=pasta"),
                          QStringLiteral("--volume"),
                          homeVolumeName(workbox.id) + QLatin1Char(':') + QDir::homePath()};
    const QMap<QString, QString> labels = labelsOf(workbox);
    for (auto label = labels.begin(); label != labels.end(); ++label) {
        arguments << QStringLiteral("--label") << label.key() + QLatin1Char('=') + label.value();
    }
    // Keeps it running until it's stopped, for what runs in it.
    arguments << image << QStringLiteral("sleep") << QStringLiteral("infinity");
    runPodman(arguments);
}

void PodmanEngine::remove(const QString &id)
{
    // --time 0: sleep, its first process, ignores SIGTERM, so Podman would
    // wait 10 seconds before killing it anyway.
    runPodman({QStringLiteral("rm"), QStringLiteral("--force"), QStringLiteral("--time"), QStringLiteral("0"), id});
    runPodman({QStringLiteral("volume"), QStringLiteral("rm"), homeVolumeName(id)});
}
