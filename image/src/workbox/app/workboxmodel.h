// The app's list (README.md), its window's and its tray icon's: every
// workbox, running or not, by id. It follows the engine's watch() and lists
// again on every change, wherever it changed, and it starts, stops and removes
// workboxes, one operation at a time per workbox, and creates them. Nothing
// blocks: each is an engine's job.
#pragma once

#include "commands.h"

#include <QAbstractListModel>

#include <memory>

class WorkboxModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int runningCount READ runningCount NOTIFY runningCountChanged)
    // Why there's no list, when workbox can't run (its settings, its engine):
    // translated, for the user. Empty otherwise.
    Q_PROPERTY(QString problem READ problem NOTIFY problemChanged)

public:
    enum class Operation {
        None,
        Starting,
        Stopping,
        Removing,
    };
    Q_ENUM(Operation)

    enum Role {
        IdRole = Qt::UserRole + 1,
        NameRole,
        ColorRole,
        RunningRole,
        // The Operation going on.
        OperationRole,
        // The last one, if it failed, and why, until the next one starts.
        FailedOperationRole,
        ErrorTextRole,
        // The whole Workbox, for C++.
        WorkboxRole,
    };

    // With the engine workbox.conf names.
    explicit WorkboxModel(QObject *parent = nullptr);
    ~WorkboxModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    int runningCount() const;
    QString problem() const;
    // The image a new workbox is created from: workbox.conf's.
    QString image() const;

    // Each one does nothing while the workbox has an operation going on.
    void start(const QString &id);
    void stop(const QString &id);
    // Without asking: the window asks first.
    void remove(const QString &id);
    // Creates the workbox id from image(), as `workbox create`; its row is
    // there as soon as the job succeeds. The job is started, and deletes
    // itself after its result. Only without a problem from the settings.
    Commands::CreateJob *create(const QString &id, const Commands::CreateOptions &options);

    // For a terminal emulator: the program and its arguments that open a
    // login shell in the running workbox, with its prompt
    // (terminalcolors.h). Empty when there's no such workbox.
    QStringList shellCommandLine(const QString &id) const;

Q_SIGNALS:
    void runningCountChanged();
    void problemChanged();

private:
    struct Row {
        Workbox workbox;
        Operation operation = Operation::None;
        Operation failedOperation = Operation::None;
        QString errorText;
    };

    void list();
    // workboxes sorted by id: rows go, come and change one by one, so a
    // row's state in QML (asking to remove it) stays while others change.
    void updateRows(const QList<Workbox> &workboxes);
    // -1 when there's none.
    int rowOf(const QString &id) const;
    void beginOperation(const QString &id, Operation operation, EngineJob *job);
    void finishOperation(const QString &id, Operation operation, const KJob *job);
    void emitRowChanged(int row);
    void updateRunningCount();
    void setProblem(const QString &problem);

    std::unique_ptr<Engine> m_engine;
    QString m_image;
    WatchJob *m_watch = nullptr;
    QList<Row> m_rows;
    int m_runningCount = 0;
    QString m_problem;
    bool m_isListing = false;
    bool m_shouldListAgain = false;
};
