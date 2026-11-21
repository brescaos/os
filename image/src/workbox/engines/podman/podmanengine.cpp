#include "podmanengine.h"

#include <KLocalizedString>

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

#include <unistd.h>

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
        Workbox workbox = workboxFromLabels(object[QLatin1String("Names")].toArray().first().toString(), labels);
        // Podman's other states (created, exited, paused…) are all stopped
        // for workbox.
        workbox.isRunning = object[QLatin1String("State")].toString() == QLatin1String("running");
        workboxes.append(workbox);
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
    if (!workbox.hasSudo) {
        return;
    }

    // The sudoers rule goes in the container's own files, written by its
    // root: a file mounted from the host wouldn't be owned by its root, and
    // sudo would refuse it. By uid, as keep-id gives the same inside.
    try {
        runPodman({QStringLiteral("start"), workbox.id});
        runPodman({QStringLiteral("exec"),
                   QStringLiteral("--user"),
                   QStringLiteral("root"),
                   workbox.id,
                   QStringLiteral("sh"),
                   QStringLiteral("-c"),
                   QStringLiteral("printf '%s\\n' \"$1\" >/etc/sudoers.d/workbox && chmod 0440 /etc/sudoers.d/workbox"),
                   QStringLiteral("sh"),
                   QStringLiteral("#%1 ALL=(ALL) NOPASSWD: ALL").arg(getuid())});
        runPodman({QStringLiteral("stop"), QStringLiteral("--time"), QStringLiteral("0"), workbox.id});
    } catch (const WorkboxError &) {
        remove(workbox.id);
        throw;
    }
}

void PodmanEngine::start(const QString &id)
{
    runPodman({QStringLiteral("start"), id});
}

// --time 0 here and in remove(): sleep, its first process, ignores SIGTERM,
// so Podman would wait 10 seconds before killing it anyway.
void PodmanEngine::stop(const QString &id)
{
    runPodman({QStringLiteral("stop"), QStringLiteral("--time"), QStringLiteral("0"), id});
}

int PodmanEngine::enter(const QString &id, const QMap<QString, QString> &environment, bool asRoot)
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

    // On this process's terminal: the shell reads and writes it itself.
    QProcess podman;
    podman.setProcessChannelMode(QProcess::ForwardedChannels);
    podman.setInputChannelMode(QProcess::ForwardedInputChannel);
    podman.start(QStringLiteral("podman"), arguments);
    if (!podman.waitForFinished(-1) && podman.error() == QProcess::FailedToStart) {
        throw WorkboxError{i18n("couldn't run podman: %1", podman.errorString())};
    }
    return podman.exitStatus() == QProcess::NormalExit ? podman.exitCode() : 1;
}

void PodmanEngine::remove(const QString &id)
{
    runPodman({QStringLiteral("rm"), QStringLiteral("--force"), QStringLiteral("--time"), QStringLiteral("0"), id});
    runPodman({QStringLiteral("volume"), QStringLiteral("rm"), homeVolumeName(id)});
}
