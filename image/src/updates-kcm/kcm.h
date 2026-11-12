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
    // arguments: what was asked for when opening it (systemsettings
    // kcm_brescaos_updates --args <action>), as requestAction().
    UpdatesKcm(QObject *parent, const KPluginMetaData &metaData, const QVariantList &arguments);

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
    // The check that install asked for ended: the page downloads and
    // installs the version it found, or installs the one downloaded.
    void installRequested();

private:
    // From the KRunner plugin (image/src/bresca-krunner/): check, a new
    // check even if there's a result already, or install, the same and then
    // installRequested(). Anything else is ignored.
    void requestAction(const QVariantList &arguments);
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
    // install was asked for, and its check hasn't ended yet.
    bool m_isInstallRequested = false;
};
