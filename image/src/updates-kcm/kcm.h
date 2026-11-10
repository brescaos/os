// System Settings > BrescaOS Updates: what the bresca-updates@ operations
// (image/src/bresca-cli/) leave in /run/bresca/updates/, for the page
// (ui/main.qml), which decides what to show, and starting them.
#pragma once

#include <KQuickConfigModule>

#include <QDateTime>
#include <QFileSystemWatcher>
#include <QHash>
#include <QSet>
#include <QTimer>
#include <QVariantMap>

class UpdatesKcm : public KQuickConfigModule
{
    Q_OBJECT
    // Each /run/bresca/updates/<name>.json, parsed, by <name>: status, check,
    // download, download-progress, install, rollback.
    Q_PROPERTY(QVariantMap results READ results NOTIFY resultsChanged)
    // The operations running, started here (also while polkit asks for the
    // password) or elsewhere (the timer, the CLI).
    Q_PROPERTY(QStringList runningOperations READ runningOperations NOTIFY runningOperationsChanged)

public:
    UpdatesKcm(QObject *parent, const KPluginMetaData &metaData);

    QVariantMap results() const
    {
        return m_results;
    }
    QStringList runningOperations() const;

    // Starts bresca-updates@<operation>, one of status, check, download,
    // download-install, install and rollback; anything else is ignored.
    Q_INVOKABLE void start(const QString &operation);
    // Plasma's restart prompt.
    Q_INVOKABLE void restart();

Q_SIGNALS:
    void resultsChanged();
    void runningOperationsChanged();

private:
    void readResults();
    void readRunningUnits();

    // inotify on the results folder: nothing runs while nothing changes.
    QFileSystemWatcher m_resultsWatcher;
    // An operation changes several files at once: read them once.
    QTimer m_readDelay;
    QVariantMap m_results;
    // When download, install and rollback last wrote their result: a new
    // one changed the versions, so status runs again.
    QHash<QString, QDateTime> m_writeTimes;
    // Started here, their systemctl start not finished yet.
    QSet<QString> m_startedOperations;
    // Running as systemd says, started from anywhere.
    QSet<QString> m_runningUnits;
};
