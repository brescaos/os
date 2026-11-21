// An engine's operation, which never blocks (README.md): a KJob. start() it
// and wait for result(), or exec() it where waiting is fine (the command
// line). On failure, error() is set and errorText() says why, translated, for
// the user. output() gives what the engine prints as it goes (an image's
// download, its errors), line by line, for whoever wants to show it.
#pragma once

#include "workbox.h"

#include <KJob>

#include <QTimer>

class EngineJob : public KJob
{
    Q_OBJECT

public:
    using KJob::KJob;

    // Lets the engine write what it prints straight to this process's
    // standard error, as it would on its own (Podman's progress bars, on a
    // terminal), instead of giving it to output(). Before start().
    void setOutputToTerminal(bool isOutputToTerminal)
    {
        m_isOutputToTerminal = isOutputToTerminal;
    }

    bool isOutputToTerminal() const
    {
        return m_isOutputToTerminal;
    }

Q_SIGNALS:
    void output(const QString &line);

protected:
    // Ends the job with message as its error.
    void fail(const QString &message)
    {
        setError(UserDefinedError);
        setErrorText(message);
        emitResult();
    }

private:
    bool m_isOutputToTerminal = false;
};

// Engine::enter()'s job: the shell's exit status, once it has ended. The
// shell failing (a non-zero status) isn't the job failing: it fails only when
// the engine can't run the shell.
class EnterJob : public EngineJob
{
    Q_OBJECT

public:
    using EngineJob::EngineJob;

    int exitStatus() const
    {
        return m_exitStatus;
    }

protected:
    void setExitStatus(int exitStatus)
    {
        m_exitStatus = exitStatus;
    }

private:
    int m_exitStatus = 0;
};

// Engine::watch()'s job: it runs until it's killed, and emits changed() when
// a workbox may have changed (created, started, stopped, removed...),
// wherever it changed (this program, a terminal, another program), without
// polling. What changed isn't said: list() again. Several changes in a row
// (one operation makes a few) give one changed(), 100 ms after the last.
class WatchJob : public EngineJob
{
    Q_OBJECT

public:
    explicit WatchJob(QObject *parent = nullptr)
        : EngineJob(parent)
    {
        m_changeDelay.setSingleShot(true);
        m_changeDelay.setInterval(100);
        connect(&m_changeDelay, &QTimer::timeout, this, &WatchJob::changed);
    }

Q_SIGNALS:
    void changed();

protected:
    // For the engine's job: something changed.
    void notifyChange()
    {
        m_changeDelay.start();
    }

    void stopNotifying()
    {
        m_changeDelay.stop();
    }

private:
    QTimer m_changeDelay;
};

// Engine::list()'s job: the workboxes, once it has succeeded.
class ListJob : public EngineJob
{
    Q_OBJECT

public:
    using EngineJob::EngineJob;

    QList<Workbox> workboxes() const
    {
        return m_workboxes;
    }

protected:
    void setWorkboxes(const QList<Workbox> &workboxes)
    {
        m_workboxes = workboxes;
    }

private:
    QList<Workbox> m_workboxes;
};
