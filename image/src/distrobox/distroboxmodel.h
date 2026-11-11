// The tray applet's list (README.md): every distrobox of the user, running or
// not, sorted by name. It lists them with podman at start and again after
// every podman event of a distrobox container, so it follows what changes
// them anywhere, and it starts, stops and removes them, one operation at a
// time per distrobox. QML creates it as DistroboxModel (com.brescaos.distrobox,
// distroboxmodelqml.h).
#pragma once

#include <QAbstractListModel>
#include <QTimer>

class QProcess;

class DistroboxModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int runningCount READ runningCount NOTIFY runningCountChanged)

public:
    enum class Operation {
        None,
        Starting,
        Stopping,
        Removing,
    };
    Q_ENUM(Operation)

    enum Role {
        NameRole = Qt::UserRole + 1,
        RunningRole,
        // Konsole's color for its tab (terminalcolors.h).
        ColorRole,
        // The Operation going on.
        OperationRole,
        // The last one, if it failed, and what the command said, until the
        // next one starts.
        FailedOperationRole,
        ErrorDetailsRole,
    };

    explicit DistroboxModel(QObject *parent = nullptr);
    ~DistroboxModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int runningCount() const;

    // Each one does nothing while the distrobox has an operation going on.
    Q_INVOKABLE void start(const QString &name);
    Q_INVOKABLE void stop(const QString &name);
    // Without asking: the applet asks first.
    Q_INVOKABLE void remove(const QString &name);

Q_SIGNALS:
    void runningCountChanged();

private:
    struct Distrobox {
        QString name;
        bool isRunning = false;
        Operation operation = Operation::None;
        Operation failedOperation = Operation::None;
        QString errorDetails;
    };

    void watchEvents();
    void list();
    // `listed` sorted by name: rows go, come and change one by one, so a
    // row's state in QML (asking to remove it) stays while others change.
    void updateRows(const QList<Distrobox> &listed);
    // -1 when there's none.
    int rowOf(const QString &name) const;
    void beginOperation(const QString &name, Operation operation, const QStringList &distroboxArguments);
    void finishOperation(const QString &name, Operation operation, bool hasSucceeded, const QString &errorDetails);
    void emitRowChanged(int row);
    void updateRunningCount();

    QList<Distrobox> m_distroboxes;
    int m_runningCount = 0;
    QProcess *m_events = nullptr;
    QTimer m_watchAgain;
    QTimer m_listSoon;
    bool m_isListing = false;
    bool m_shouldListAgain = false;
};
