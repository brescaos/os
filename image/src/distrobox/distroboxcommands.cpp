#include "distroboxcommands.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

void runProgram(QObject *context,
                const QString &program,
                const QStringList &arguments,
                const std::function<void(bool hasSucceeded, const QString &output, const QString &errors)> &done)
{
    auto *process = new QProcess(context);
    QObject::connect(process, &QProcess::finished, context, [process, done](int exitCode, QProcess::ExitStatus exitStatus) {
        done(exitStatus == QProcess::NormalExit && exitCode == 0,
             QString::fromUtf8(process->readAllStandardOutput()),
             QString::fromUtf8(process->readAllStandardError()).trimmed());
        process->deleteLater();
    });
    QObject::connect(process, &QProcess::errorOccurred, context, [process, done](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            done(false, QString(), process->errorString());
            process->deleteLater();
        }
    });
    process->start(program, arguments);
}

namespace
{

void runDistrobox(QObject *context, const QStringList &arguments, const DistroboxCommands::Finished &finished)
{
    runProgram(context, QStringLiteral("distrobox"), arguments, [finished](bool hasSucceeded, const QString &, const QString &errors) {
        finished(hasSucceeded, errors);
    });
}

}

namespace DistroboxCommands
{

QStringList listArguments()
{
    return {QStringLiteral("ps"),
            QStringLiteral("--all"),
            QStringLiteral("--filter"),
            QStringLiteral("label=manager=distrobox"),
            QStringLiteral("--format"),
            QStringLiteral("json")};
}

QList<ListedDistrobox> parseListing(const QString &json)
{
    QList<ListedDistrobox> distroboxes;
    for (const QJsonValue &container : QJsonDocument::fromJson(json.toUtf8()).array()) {
        const QJsonObject object = container.toObject();
        distroboxes.append({.name = object[QLatin1String("Names")].toArray().first().toString(),
                            .isRunning = object[QLatin1String("State")].toString() == QLatin1String("running")});
    }
    return distroboxes;
}

void create(QObject *context, const QString &name, const Finished &finished)
{
    runDistrobox(context, {QStringLiteral("create"), QStringLiteral("--yes"), QStringLiteral("--name"), name}, finished);
}

void start(QObject *context, const QString &name, const Finished &finished)
{
    runDistrobox(context, {QStringLiteral("enter"), name, QStringLiteral("--"), QStringLiteral("true")}, finished);
}

void stop(QObject *context, const QString &name, const Finished &finished)
{
    runDistrobox(context, {QStringLiteral("stop"), QStringLiteral("--yes"), name}, [context, name, finished](bool hasSucceeded, const QString &errors) {
        if (hasSucceeded) {
            finished(true, QString());
            return;
        }
        // With a terminal open in it, podman stops it but then fails, "has
        // active exec sessions, refusing to clean up": stopped is stopped.
        runProgram(context,
                   QStringLiteral("podman"),
                   {QStringLiteral("inspect"), QStringLiteral("--format"), QStringLiteral("{{.State.Running}}"), name},
                   [finished, errors](bool hasInspected, const QString &output, const QString &) {
                       finished(hasInspected && output.trimmed() == QLatin1String("false"), errors);
                   });
    });
}

void remove(QObject *context, const QString &name, const Finished &finished)
{
    runDistrobox(context, {QStringLiteral("rm"), QStringLiteral("--force"), name}, finished);
}

}
