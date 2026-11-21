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
        workbox.image = object[QLatin1String("Image")].toString();
        workboxes.append(workbox);
    }
    return workboxes;
}

// podman exec's arguments for enter() and shellCommandLine(). keep-id runs
// it as the user unless --user says otherwise. TERM as the terminal's, for
// the shell's programs.
QStringList execArguments(const QString &id, const QStringList &command, const QMap<QString, QString> &environment, bool asRoot, bool hasTerminal)
{
    QStringList arguments{QStringLiteral("exec"), QStringLiteral("--interactive")};
    if (hasTerminal) {
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
    // The command straight, as Toolbox and Distrobox do: no login shell.
    arguments << id << (command.isEmpty() ? QStringList{QStringLiteral("bash"), QStringLiteral("--login")} : command);
    return arguments;
}

// --time 0: sleep, a workbox's first process, ignores SIGTERM, so Podman
// would wait 10 seconds before killing it anyway.
QStringList stopCommand(const QString &id)
{
    return {QStringLiteral("stop"), QStringLiteral("--time"), QStringLiteral("0"), id};
}

// The user's Wayland session in the workbox, as Flatpak gives it to an app:
// only its socket, not the user's whole runtime folder (PipeWire, the session
// bus, the SSH agent...), at a path of its own that WAYLAND_DISPLAY names
// (libwayland takes an absolute one); a runtime folder of the workbox's own,
// the user's, 0700, as apps want one; the GPU; Qt on Wayland, as there's no
// X11. z: SELinux labels the socket so the workbox can use it, as a shared
// folder; Podman mounts the socket when the workbox starts, so after a new
// login it's the new session's.
QStringList waylandArguments()
{
    const QString display = qEnvironmentVariable("WAYLAND_DISPLAY");
    const QString socket = QDir::isAbsolutePath(display) ? display : qEnvironmentVariable("XDG_RUNTIME_DIR") + QLatin1Char('/') + display;
    const QString runtimeFolder = QStringLiteral("/run/user/%1").arg(getuid());
    return {QStringLiteral("--volume"),
            socket + QStringLiteral(":/run/workbox/wayland-0:z"),
            QStringLiteral("--env"),
            QStringLiteral("WAYLAND_DISPLAY=/run/workbox/wayland-0"),
            // U: Podman gives it to the container's user, the user (keep-id);
            // --tmpfs takes no owner.
            QStringLiteral("--mount"),
            QStringLiteral("type=tmpfs,destination=%1,tmpfs-mode=0700,U=true").arg(runtimeFolder),
            QStringLiteral("--env"),
            QStringLiteral("XDG_RUNTIME_DIR=") + runtimeFolder,
            QStringLiteral("--device"),
            QStringLiteral("/dev/dri"),
            QStringLiteral("--env"),
            QStringLiteral("QT_QPA_PLATFORM=wayland")};
}

QList<QStringList> removeCommands(const Workbox &workbox)
{
    // --volumes: its anonymous volumes go with it; then its home and its
    // named ones.
    QList<QStringList> commands{{QStringLiteral("rm"), QStringLiteral("--force"), QStringLiteral("--time"), QStringLiteral("0"), QStringLiteral("--volumes"), workbox.id}};
    QStringList volumes{QStringLiteral("volume"), QStringLiteral("rm"), volumeName(workbox.id, homeVolume)};
    for (const Volume &volume : workbox.volumes) {
        if (!volume.name.isEmpty()) {
            volumes << volumeName(workbox.id, volume.name);
        }
    }
    commands.append(volumes);
    return commands;
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
                          volumeName(workbox.id, homeVolume) + QLatin1Char(':') + QDir::homePath()};
    // z: SELinux labels a shared folder so containers can read it, and so can
    // every workbox that shares it (Z would give it to one). Its volumes,
    // named or not, Podman labels itself, and creates if they're missing.
    for (const Volume &volume : workbox.volumes) {
        QString argument;
        if (!volume.source.isEmpty()) {
            argument = volume.source + QLatin1Char(':') + volume.target + (volume.isReadOnly ? QStringLiteral(":z,ro") : QStringLiteral(":z"));
        } else {
            argument = (volume.name.isEmpty() ? QString() : volumeName(workbox.id, volume.name) + QLatin1Char(':')) + volume.target
                + (volume.isReadOnly ? QStringLiteral(":ro") : QString());
        }
        arguments << QStringLiteral("--volume") << argument;
    }
    if (workbox.hasWayland) {
        arguments << waylandArguments();
    }
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
    job->setCleanupCommands(removeCommands(workbox));
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

EngineJob *PodmanEngine::remove(const Workbox &workbox)
{
    return new PodmanJob(removeCommands(workbox));
}

EnterJob *PodmanEngine::enter(const QString &id, const QStringList &command, const QMap<QString, QString> &environment, bool asRoot)
{
    // --tty only on a terminal: podman refuses it without one.
    return new PodmanEnterJob(execArguments(id, command, environment, asRoot, isatty(STDIN_FILENO)));
}

QStringList PodmanEngine::shellCommandLine(const QString &id, const QMap<QString, QString> &environment)
{
    return QStringList{QStringLiteral("podman")} + execArguments(id, {}, environment, false, true);
}
