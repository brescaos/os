#include "podmanengine.h"

#include "podmanjob.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <unistd.h>

namespace
{

QList<Workbox> workboxesFromJson(const QByteArray &json)
{
    QList<Workbox> workboxes;
    for (const QJsonValue &container : QJsonDocument::fromJson(json).array()) {
        const QJsonObject object = container.toObject();
        QMap<QString, QString> labels;
        const QJsonObject labelObject = object[QLatin1String("Labels")].toObject();
        for (auto label = labelObject.begin(); label != labelObject.end(); ++label) {
            labels.insert(label.key(), label.value().toString());
        }
        Workbox workbox = workboxFromLabels(object[QLatin1String("Names")].toArray().first().toString(), labels);
        // Podman's other states (created, exited, paused…) are all stopped
        // for workbox.
        workbox.isRunning = object[QLatin1String("State")].toString() == QLatin1String("running");
        workboxes.append(workbox);
    }
    return workboxes;
}

// --time 0: sleep, a workbox's first process, ignores SIGTERM, so Podman
// would wait 10 seconds before killing it anyway.
QStringList stopCommand(const QString &id)
{
    return {QStringLiteral("stop"), QStringLiteral("--time"), QStringLiteral("0"), id};
}

QList<QStringList> removeCommands(const QString &id)
{
    return {{QStringLiteral("rm"), QStringLiteral("--force"), QStringLiteral("--time"), QStringLiteral("0"), id},
            {QStringLiteral("volume"), QStringLiteral("rm"), homeVolumeName(id)}};
}

}

ListJob *PodmanEngine::list()
{
    auto *job = new PodmanJob({{QStringLiteral("ps"),
                                QStringLiteral("--all"),
                                QStringLiteral("--filter"),
                                QStringLiteral("label=") + nameLabel,
                                QStringLiteral("--format"),
                                QStringLiteral("json")}});
    job->setWorkboxesParser(workboxesFromJson);
    return job;
}

WatchJob *PodmanEngine::watch()
{
    return new PodmanWatchJob({QStringLiteral("events"),
                               QStringLiteral("--format"),
                               QStringLiteral("json"),
                               QStringLiteral("--filter"),
                               QStringLiteral("type=container"),
                               QStringLiteral("--filter"),
                               QStringLiteral("label=") + nameLabel});
}

EngineJob *PodmanEngine::create(const Workbox &workbox, const QString &image)
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
    if (!workbox.hasSudo) {
        return new PodmanJob({arguments});
    }

    // The sudoers rule goes in the container's own files, written by its
    // root: a file mounted from the host wouldn't be owned by its root, and
    // sudo would refuse it. By uid, as keep-id gives the same inside. If
    // that fails, nothing of the workbox stays.
    auto *job = new PodmanJob({arguments,
                               {QStringLiteral("start"), workbox.id},
                               {QStringLiteral("exec"),
                                QStringLiteral("--user"),
                                QStringLiteral("root"),
                                workbox.id,
                                QStringLiteral("sh"),
                                QStringLiteral("-c"),
                                QStringLiteral("printf '%s\\n' \"$1\" >/etc/sudoers.d/workbox && chmod 0440 /etc/sudoers.d/workbox"),
                                QStringLiteral("sh"),
                                QStringLiteral("#%1 ALL=(ALL) NOPASSWD: ALL").arg(getuid())},
                               stopCommand(workbox.id)});
    job->setCleanupCommands(removeCommands(workbox.id));
    return job;
}

EngineJob *PodmanEngine::start(const QString &id)
{
    return new PodmanJob({{QStringLiteral("start"), id}});
}

EngineJob *PodmanEngine::stop(const QString &id)
{
    return new PodmanJob({stopCommand(id)});
}

EngineJob *PodmanEngine::remove(const QString &id)
{
    return new PodmanJob(removeCommands(id));
}

EnterJob *PodmanEngine::enter(const QString &id, const QMap<QString, QString> &environment, bool asRoot)
{
    // keep-id runs it as the user unless --user says otherwise. --tty only on
    // a terminal: podman refuses it without one. TERM as the terminal's, for
    // the shell's programs.
    QStringList arguments{QStringLiteral("exec"), QStringLiteral("--interactive")};
    if (isatty(STDIN_FILENO)) {
        arguments << QStringLiteral("--tty");
    }
    if (asRoot) {
        arguments << QStringLiteral("--user") << QStringLiteral("root") << QStringLiteral("--workdir") << QStringLiteral("/root");
    } else {
        arguments << QStringLiteral("--workdir") << QDir::homePath();
    }
    arguments << QStringLiteral("--env") << QStringLiteral("TERM");
    for (auto variable = environment.begin(); variable != environment.end(); ++variable) {
        arguments << QStringLiteral("--env") << variable.key() + QLatin1Char('=') + variable.value();
    }
    arguments << id << QStringLiteral("bash") << QStringLiteral("--login");
    return new PodmanEnterJob(arguments);
}
