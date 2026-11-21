#include "podmanjob.h"

#include <KLocalizedString>

#include <QProcess>

PodmanJob::PodmanJob(const QList<QStringList> &commands, QObject *parent)
    : ListJob(parent)
    , m_pendingCommands(commands)
{
}

void PodmanJob::setCleanupCommands(const QList<QStringList> &commands)
{
    m_cleanupCommands = commands;
}

void PodmanJob::setWorkboxesParser(const std::function<QList<Workbox>(const QByteArray &)> &parser)
{
    m_workboxesParser = parser;
}

void PodmanJob::start()
{
    // After start() returns, as a KJob's result always comes.
    QMetaObject::invokeMethod(this, &PodmanJob::runNextCommand, Qt::QueuedConnection);
}

void PodmanJob::runNextCommand()
{
    if (m_pendingCommands.isEmpty()) {
        if (!m_failure.isEmpty()) {
            fail(m_failure);
            return;
        }
        if (m_workboxesParser) {
            setWorkboxes(m_workboxesParser(m_lastOutput));
        }
        emitResult();
        return;
    }

    const QStringList arguments = m_pendingCommands.takeFirst();
    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardError, this, [this] {
        m_errorOutput += m_process->readAllStandardError();
        emitCompleteLines(false);
    });
    connect(m_process, &QProcess::finished, this, [this, arguments](int exitCode, QProcess::ExitStatus exitStatus) {
        finishCommand(exitStatus == QProcess::NormalExit && exitCode == 0, i18n("podman %1 failed", arguments.join(QLatin1Char(' '))));
    });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            finishCommand(false, i18n("couldn't run podman: %1", m_process->errorString()));
        }
    });
    m_process->start(QStringLiteral("podman"), arguments);
}

void PodmanJob::finishCommand(bool hasSucceeded, const QString &failure)
{
    m_errorOutput += m_process->readAllStandardError();
    emitCompleteLines(true);
    m_lastOutput = m_process->readAllStandardOutput();
    m_process->deleteLater();
    m_process = nullptr;

    if (hasSucceeded) {
        m_hasACommandSucceeded = true;
    } else if (m_failure.isEmpty()) {
        m_failure = failure;
        m_pendingCommands = m_hasACommandSucceeded ? m_cleanupCommands : QList<QStringList>();
    }
    runNextCommand();
}

void PodmanJob::emitCompleteLines(bool isLast)
{
    qsizetype end;
    while ((end = m_errorOutput.indexOf('\n')) >= 0) {
        Q_EMIT output(QString::fromUtf8(m_errorOutput.left(end)));
        m_errorOutput.remove(0, end + 1);
    }
    if (isLast && !m_errorOutput.isEmpty()) {
        Q_EMIT output(QString::fromUtf8(m_errorOutput));
        m_errorOutput.clear();
    }
}

bool PodmanJob::doKill()
{
    m_pendingCommands.clear();
    if (m_process) {
        m_process->disconnect(this);
        m_process->kill();
    }
    return true;
}

PodmanEnterJob::PodmanEnterJob(const QStringList &arguments, QObject *parent)
    : EnterJob(parent)
    , m_arguments(arguments)
{
}

void PodmanEnterJob::start()
{
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::ForwardedChannels);
    m_process->setInputChannelMode(QProcess::ForwardedInputChannel);
    connect(m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        setExitStatus(exitStatus == QProcess::NormalExit ? exitCode : 1);
        emitResult();
    });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            fail(i18n("couldn't run podman: %1", m_process->errorString()));
        }
    });
    m_process->start(QStringLiteral("podman"), m_arguments);
}

bool PodmanEnterJob::doKill()
{
    if (m_process) {
        m_process->disconnect(this);
        m_process->kill();
    }
    return true;
}

PodmanWatchJob::PodmanWatchJob(const QStringList &arguments, std::chrono::milliseconds restartDelay, QObject *parent)
    : WatchJob(parent)
    , m_arguments(arguments)
{
    m_restart.setSingleShot(true);
    m_restart.setInterval(restartDelay);
    connect(&m_restart, &QTimer::timeout, this, [this] {
        startEvents();
        notifyChange();
    });
}

void PodmanWatchJob::start()
{
    startEvents();
}

void PodmanWatchJob::startEvents()
{
    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this] {
        // Only whether there was an event: what it says, list() reads again.
        if (!m_process->readAllStandardOutput().isEmpty()) {
            notifyChange();
        }
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this] {
        for (const QByteArray &line : m_process->readAllStandardError().trimmed().split('\n')) {
            Q_EMIT output(QString::fromUtf8(line));
        }
    });
    const auto restartLater = [this] {
        m_process->deleteLater();
        m_process = nullptr;
        m_restart.start();
    };
    connect(m_process, &QProcess::finished, this, restartLater);
    connect(m_process, &QProcess::errorOccurred, this, [restartLater](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            restartLater();
        }
    });
    m_process->start(QStringLiteral("podman"), m_arguments);
}

bool PodmanWatchJob::doKill()
{
    m_restart.stop();
    stopNotifying();
    if (m_process) {
        m_process->disconnect(this);
        m_process->kill();
    }
    return true;
}
