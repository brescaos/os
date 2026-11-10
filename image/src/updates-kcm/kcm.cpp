#include "kcm.h"

#include <KPluginFactory>

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>

// The metadata, kcm_brescaos_updates.json, is kcm_brescaos_updates.json.in
// translated by image/build/compile_translations.sh.
K_PLUGIN_CLASS_WITH_JSON(UpdatesKcm, "kcm_brescaos_updates.json")

namespace
{
const QString resultsFolder = QStringLiteral("/run/bresca/updates");
const QStringList operations = {
    QStringLiteral("status"),
    QStringLiteral("check"),
    QStringLiteral("download"),
    QStringLiteral("download-install"),
    QStringLiteral("install"),
    QStringLiteral("rollback"),
};
const QStringList versionChangingResults = {
    QStringLiteral("download"),
    QStringLiteral("install"),
    QStringLiteral("rollback"),
};
}

UpdatesKcm::UpdatesKcm(QObject *parent, const KPluginMetaData &metaData)
    : KQuickConfigModule(parent, metaData)
{
    // Everything applies at once: no Apply, Reset or Defaults.
    setButtons(NoAdditionalButton);

    m_readDelay.setSingleShot(true);
    m_readDelay.setInterval(100);
    connect(&m_readDelay, &QTimer::timeout, this, [this] {
        readResults();
        readRunningUnits();
    });
    // Created at boot (bresca-updates.tmpfiles).
    m_resultsWatcher.addPath(resultsFolder);
    connect(&m_resultsWatcher, &QFileSystemWatcher::directoryChanged, &m_readDelay, qOverload<>(&QTimer::start));

    readResults();
    readRunningUnits();
    start(QStringLiteral("status"));
    // None since the boot: the timer's first one is minutes away.
    if (!m_results.contains(QStringLiteral("check"))) {
        start(QStringLiteral("check"));
    }
}

QStringList UpdatesKcm::runningOperations() const
{
    return (m_startedOperations | m_runningUnits).values();
}

void UpdatesKcm::start(const QString &operation)
{
    if (!operations.contains(operation) || m_startedOperations.contains(operation)) {
        return;
    }
    // systemctl start returns when the operation ends, so its end is known
    // without asking. polkit asks for the password first, except for status
    // and check (bresca-updates.rules); cancelling it ends it at once.
    auto *systemctl = new QProcess(this);
    connect(systemctl, &QProcess::finished, this, [this, systemctl, operation] {
        systemctl->deleteLater();
        m_startedOperations.remove(operation);
        Q_EMIT runningOperationsChanged();
        readResults();
        readRunningUnits();
    });
    systemctl->start(QStringLiteral("/usr/bin/systemctl"),
                     {QStringLiteral("start"), QStringLiteral("bresca-updates@%1.service").arg(operation)});
    m_startedOperations.insert(operation);
    Q_EMIT runningOperationsChanged();
}

void UpdatesKcm::restart()
{
    const QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("org.kde.LogoutPrompt"),
                                                             QStringLiteral("/LogoutPrompt"),
                                                             QStringLiteral("org.kde.LogoutPrompt"),
                                                             QStringLiteral("promptReboot"));
    QDBusConnection::sessionBus().asyncCall(call);
}

void UpdatesKcm::readResults()
{
    QVariantMap results;
    bool isVersionChanged = false;
    const QFileInfoList files = QDir(resultsFolder).entryInfoList({QStringLiteral("*.json")}, QDir::Files);
    for (const QFileInfo &file : files) {
        const QString name = file.completeBaseName();
        QFile result(file.filePath());
        if (!result.open(QIODevice::ReadOnly)) {
            continue;
        }
        results.insert(name, QJsonDocument::fromJson(result.readAll()).toVariant());
        if (versionChangingResults.contains(name) && m_writeTimes.value(name) != file.lastModified()) {
            m_writeTimes.insert(name, file.lastModified());
            isVersionChanged = true;
        }
    }
    if (results != m_results) {
        m_results = results;
        Q_EMIT resultsChanged();
    }
    if (isVersionChanged) {
        start(QStringLiteral("status"));
    }
}

void UpdatesKcm::readRunningUnits()
{
    // A oneshot unit is activating while its operation runs.
    QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                       QStringLiteral("/org/freedesktop/systemd1"),
                                                       QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                       QStringLiteral("ListUnitsByPatterns"));
    call << QStringList{QStringLiteral("activating")} << QStringList{QStringLiteral("bresca-updates@*.service")};
    auto *pendingReply = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(call), this);
    connect(pendingReply, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *reply) {
        reply->deleteLater();
        if (reply->isError()) {
            return;
        }
        // a(ssssssouso): name, description, load, active and sub states,
        // followed unit, path, job id, type and path.
        QSet<QString> runningUnits;
        const QDBusArgument units = reply->reply().arguments().value(0).value<QDBusArgument>();
        units.beginArray();
        while (!units.atEnd()) {
            QString name, description, loadState, activeState, subState, followedUnit, jobType;
            QDBusObjectPath path, jobPath;
            uint jobId;
            units.beginStructure();
            units >> name >> description >> loadState >> activeState >> subState >> followedUnit >> path >> jobId >> jobType >> jobPath;
            units.endStructure();
            // bresca-updates@<operation>.service
            runningUnits.insert(name.section(QLatin1Char('@'), 1).section(QLatin1Char('.'), 0, 0));
        }
        units.endArray();
        if (runningUnits != m_runningUnits) {
            m_runningUnits = runningUnits;
            Q_EMIT runningOperationsChanged();
        }
    });
}

#include "kcm.moc"
