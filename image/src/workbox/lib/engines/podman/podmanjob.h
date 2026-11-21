// Podman's commands as an EngineJob (README.md): run one after another with
// QProcess's signals, never waiting. Each one's standard error goes to
// output() line by line as it comes (an image's download, Podman's errors),
// or straight to this process's with setOutputToTerminal().
// It fails at the first command that fails, after its cleanup commands, if
// any, whose own failures are ignored. The cleanup undoes what the commands
// before did, so it doesn't run when the first one fails: there's nothing to
// undo, and what's there isn't the job's (a container with that name).
#pragma once

#include "enginejob.h"

#include <QTimer>

#include <chrono>
#include <functional>

class QProcess;

// A ListJob so that list() can return it too: its workboxes() are empty
// unless it was given a parser.
class PodmanJob : public ListJob
{
    Q_OBJECT

public:
    explicit PodmanJob(const QList<QStringList> &commands, QObject *parent = nullptr);

    void setCleanupCommands(const QList<QStringList> &commands);

    // Reads the workboxes from the last command's standard output.
    void setWorkboxesParser(const std::function<QList<Workbox>(const QByteArray &)> &parser);

    void start() override;

protected:
    bool doKill() override;

private:
    void runNextCommand();
    void finishCommand(bool hasSucceeded, const QString &failure);
    void emitCompleteLines(bool isLast);

    QList<QStringList> m_pendingCommands;
    QList<QStringList> m_cleanupCommands;
    std::function<QList<Workbox>(const QByteArray &)> m_workboxesParser;
    QProcess *m_process = nullptr;
    QByteArray m_errorOutput;
    QByteArray m_lastOutput;
    bool m_hasACommandSucceeded = false;
    // Why it fails, while its cleanup commands run.
    QString m_failure;
};

// podman exec's shell on this process's terminal, as an EnterJob: podman
// reads and writes the terminal itself, so there's no output(). It ends when
// the shell exits, with its exit status; it fails only when podman can't run.
class PodmanEnterJob : public EnterJob
{
    Q_OBJECT

public:
    explicit PodmanEnterJob(const QStringList &arguments, QObject *parent = nullptr);

    void start() override;

protected:
    bool doKill() override;

private:
    QStringList m_arguments;
    QProcess *m_process = nullptr;
};

// podman events, following the workboxes' containers, as a WatchJob: each
// event Podman prints is a change. If podman events ends (Podman updated, a
// crash), it starts it again restartDelay later and says something changed,
// as it may have missed some. Its standard error goes to output().
class PodmanWatchJob : public WatchJob
{
    Q_OBJECT

public:
    explicit PodmanWatchJob(const QStringList &arguments, std::chrono::milliseconds restartDelay = std::chrono::seconds(10), QObject *parent = nullptr);

    void start() override;

protected:
    bool doKill() override;

private:
    void startEvents();

    QStringList m_arguments;
    QProcess *m_process = nullptr;
    QTimer m_restart;
};
