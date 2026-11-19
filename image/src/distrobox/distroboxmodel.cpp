#include "distroboxmodel.h"

#include "terminalcolors.h"

#include <QGuiApplication>
#include <QPalette>
#include <QProcess>

#include <algorithm>

using namespace std::chrono_literals;

DistroboxModel::DistroboxModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // If podman events ends (a crash, podman missing).
    m_watchAgain.setSingleShot(true);
    m_watchAgain.setInterval(10s);
    connect(&m_watchAgain, &QTimer::timeout, this, &DistroboxModel::watchEvents);
    // Starting a distrobox gives several events in a row: one listing.
    m_listSoon.setSingleShot(true);
    m_listSoon.setInterval(100ms);
    connect(&m_listSoon, &QTimer::timeout, this, &DistroboxModel::list);
    watchEvents();
}

DistroboxModel::~DistroboxModel()
{
    if (m_events) {
        m_events->disconnect(this);
        m_events->kill();
        m_events->waitForFinished(1000);
    }
}

int DistroboxModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_distroboxes.size());
}

QVariant DistroboxModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }
    const Distrobox &distrobox = m_distroboxes[index.row()];
    switch (role) {
    case NameRole:
        return distrobox.name;
    case RunningRole:
        return distrobox.isRunning;
    case ColorRole: {
        // Konsole's accent: its palette's highlight, the same as this
        // process's in Plasma.
        const QColor accent = qGuiApp ? qGuiApp->palette().color(QPalette::Active, QPalette::Highlight) : QColor();
        return TerminalColors::konsoleTabColor(distrobox.name, accent);
    }
    case OperationRole:
        return static_cast<int>(distrobox.operation);
    case FailedOperationRole:
        return static_cast<int>(distrobox.failedOperation);
    case ErrorDetailsRole:
        return distrobox.errorDetails;
    }
    return {};
}

QHash<int, QByteArray> DistroboxModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {RunningRole, "running"},
        {ColorRole, "color"},
        {OperationRole, "operation"},
        {FailedOperationRole, "failedOperation"},
        {ErrorDetailsRole, "errorDetails"},
    };
}

int DistroboxModel::runningCount() const
{
    return m_runningCount;
}

void DistroboxModel::start(const QString &name)
{
    // The spinner lasts as long as the first start's setup.
    beginOperation(name, Operation::Starting, &DistroboxCommands::start);
}

void DistroboxModel::stop(const QString &name)
{
    beginOperation(name, Operation::Stopping, &DistroboxCommands::stop);
}

void DistroboxModel::remove(const QString &name)
{
    beginOperation(name, Operation::Removing, &DistroboxCommands::remove);
}

void DistroboxModel::watchEvents()
{
    if (!m_events) {
        m_events = new QProcess(this);
        m_events->setProcessChannelMode(QProcess::ForwardedErrorChannel);
        // Each line is an event; which one doesn't matter, the listing says.
        connect(m_events, &QProcess::readyReadStandardOutput, this, [this] {
            m_events->readAllStandardOutput();
            m_listSoon.start();
        });
        connect(m_events, &QProcess::finished, this, [this] {
            qWarning("podman events ended: watching again in 10 s");
            m_watchAgain.start();
        });
        connect(m_events, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                qWarning("podman events didn't start: %s", qPrintable(m_events->errorString()));
                m_watchAgain.start();
            }
        });
    }
    m_events->start(QStringLiteral("podman"),
                    {QStringLiteral("events"),
                     QStringLiteral("--filter"),
                     QStringLiteral("type=container"),
                     QStringLiteral("--filter"),
                     QStringLiteral("label=manager=distrobox")});
    // Whatever changed while it wasn't watching.
    list();
}

void DistroboxModel::list()
{
    if (m_isListing) {
        m_shouldListAgain = true;
        return;
    }
    m_isListing = true;
    runProgram(this, QStringLiteral("podman"), DistroboxCommands::listArguments(), [this](bool hasSucceeded, const QString &output, const QString &errors) {
        m_isListing = false;
        if (hasSucceeded) {
            QList<Distrobox> listed;
            for (const DistroboxCommands::ListedDistrobox &distrobox : DistroboxCommands::parseListing(output)) {
                listed.append({.name = distrobox.name, .isRunning = distrobox.isRunning});
            }
            std::ranges::sort(listed, {}, &Distrobox::name);
            updateRows(listed);
        } else {
            // The last listing stays.
            qWarning("podman ps failed: %s", qPrintable(errors));
        }
        if (m_shouldListAgain) {
            m_shouldListAgain = false;
            list();
        }
    });
}

void DistroboxModel::updateRows(const QList<Distrobox> &listed)
{
    int row = 0;
    for (const Distrobox &distrobox : listed) {
        while (row < m_distroboxes.size() && m_distroboxes[row].name < distrobox.name) {
            beginRemoveRows(QModelIndex(), row, row);
            m_distroboxes.removeAt(row);
            endRemoveRows();
        }
        if (row < m_distroboxes.size() && m_distroboxes[row].name == distrobox.name) {
            if (m_distroboxes[row].isRunning != distrobox.isRunning) {
                m_distroboxes[row].isRunning = distrobox.isRunning;
                emitRowChanged(row);
            }
        } else {
            beginInsertRows(QModelIndex(), row, row);
            m_distroboxes.insert(row, distrobox);
            endInsertRows();
        }
        ++row;
    }
    if (row < m_distroboxes.size()) {
        beginRemoveRows(QModelIndex(), row, static_cast<int>(m_distroboxes.size()) - 1);
        m_distroboxes.resize(row);
        endRemoveRows();
    }
    updateRunningCount();
}

int DistroboxModel::rowOf(const QString &name) const
{
    const auto found = std::ranges::find(m_distroboxes, name, &Distrobox::name);
    return found == m_distroboxes.end() ? -1 : static_cast<int>(found - m_distroboxes.begin());
}

void DistroboxModel::beginOperation(const QString &name, Operation operation, DistroboxCommands::Command command)
{
    const int row = rowOf(name);
    if (row < 0 || m_distroboxes[row].operation != Operation::None) {
        return;
    }
    Distrobox &distrobox = m_distroboxes[row];
    distrobox.operation = operation;
    distrobox.failedOperation = Operation::None;
    distrobox.errorDetails.clear();
    emitRowChanged(row);
    command(this, name, [this, name, operation](bool hasSucceeded, const QString &errors) {
        finishOperation(name, operation, hasSucceeded, errors);
    });
}

// Its new state at once, not after podman's events: the spinner gives way to
// the right button.
void DistroboxModel::finishOperation(const QString &name, Operation operation, bool hasSucceeded, const QString &errorDetails)
{
    const int row = rowOf(name);
    if (row < 0) {
        return;
    }
    if (hasSucceeded && operation == Operation::Removing) {
        beginRemoveRows(QModelIndex(), row, row);
        m_distroboxes.removeAt(row);
        endRemoveRows();
        updateRunningCount();
        return;
    }
    Distrobox &distrobox = m_distroboxes[row];
    distrobox.operation = Operation::None;
    if (hasSucceeded) {
        distrobox.isRunning = operation == Operation::Starting;
    } else {
        distrobox.failedOperation = operation;
        distrobox.errorDetails = errorDetails;
    }
    emitRowChanged(row);
    updateRunningCount();
}

void DistroboxModel::emitRowChanged(int row)
{
    const QModelIndex changed = index(row);
    Q_EMIT dataChanged(changed, changed);
}

void DistroboxModel::updateRunningCount()
{
    const int runningCount = static_cast<int>(std::ranges::count(m_distroboxes, true, &Distrobox::isRunning));
    if (runningCount != m_runningCount) {
        m_runningCount = runningCount;
        Q_EMIT runningCountChanged();
    }
}
