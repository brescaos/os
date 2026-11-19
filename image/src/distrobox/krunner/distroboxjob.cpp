#include "distroboxjob.h"

DistroboxJob::DistroboxJob(DistroboxCommands::Command command, const QString &name, const QString &title, QObject *parent)
    : KJob(parent)
    , m_command(command)
    , m_name(name)
    , m_title(title)
{
}

void DistroboxJob::start()
{
    Q_EMIT description(this, m_title);
    m_command(this, m_name, [this](bool hasSucceeded, const QString &errors) {
        if (!hasSucceeded) {
            setError(KJob::UserDefinedError);
            setErrorText(errors);
        }
        emitResult();
    });
}
