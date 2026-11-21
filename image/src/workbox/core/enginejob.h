// An engine's operation, which never blocks (README.md): a KJob. start() it
// and wait for result(), or exec() it where waiting is fine (the command
// line). On failure, error() is set and errorText() says why, translated, for
// the user. output() gives what the engine prints as it goes (an image's
// download, its errors), line by line, for whoever wants to show it.
#pragma once

#include "workbox.h"

#include <KJob>

class EngineJob : public KJob
{
    Q_OBJECT

public:
    using KJob::KJob;

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
