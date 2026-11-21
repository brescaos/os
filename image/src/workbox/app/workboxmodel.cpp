#include "workboxmodel.h"

#include "commands.h"
#include "engines.h"
#include "settings.h"
#include "terminalcolors.h"

#include <algorithm>

WorkboxModel::WorkboxModel(QObject *parent)
    : QAbstractListModel(parent)
{
    try {
        m_engine = engineNamed(readSettings(settingsFiles()).engine);
    } catch (const WorkboxError &error) {
        setProblem(error.message);
        return;
    }
    // Before the first list(), so no change between them is missed.
    m_watch = m_engine->watch();
    m_watch->setAutoDelete(false);
    connect(m_watch, &WatchJob::changed, this, &WorkboxModel::list);
    m_watch->start();
    list();
}

WorkboxModel::~WorkboxModel()
{
    if (m_watch) {
        m_watch->kill();
        delete m_watch;
    }
}

int WorkboxModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant WorkboxModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }
    const Row &row = m_rows[index.row()];
    switch (role) {
    case IdRole:
        return row.workbox.id;
    case NameRole:
        return row.workbox.name;
    case ColorRole:
        return row.workbox.color;
    case RunningRole:
        return row.workbox.isRunning;
    case OperationRole:
        return static_cast<int>(row.operation);
    case FailedOperationRole:
        return static_cast<int>(row.failedOperation);
    case ErrorTextRole:
        return row.errorText;
    case WorkboxRole:
        return QVariant::fromValue(row.workbox);
    }
    return {};
}

int WorkboxModel::runningCount() const
{
    return m_runningCount;
}

QString WorkboxModel::problem() const
{
    return m_problem;
}

void WorkboxModel::start(const QString &id)
{
    if (m_engine) {
        beginOperation(id, Operation::Starting, m_engine->start(id));
    }
}

void WorkboxModel::stop(const QString &id)
{
    if (m_engine) {
        beginOperation(id, Operation::Stopping, m_engine->stop(id));
    }
}

void WorkboxModel::remove(const QString &id)
{
    const int row = rowOf(id);
    if (m_engine && row >= 0) {
        beginOperation(id, Operation::Removing, m_engine->remove(m_rows[row].workbox));
    }
}

QStringList WorkboxModel::shellCommandLine(const QString &id) const
{
    const int row = rowOf(id);
    if (!m_engine || row < 0) {
        return {};
    }
    const Workbox &workbox = m_rows[row].workbox;
    return m_engine->shellCommandLine(id, promptEnvironment(workbox.name, terminalColorsFor(workbox.color)));
}

void WorkboxModel::list()
{
    if (m_isListing) {
        m_shouldListAgain = true;
        return;
    }
    m_isListing = true;
    ListJob *job = m_engine->list();
    connect(job, &KJob::result, this, [this, job] {
        m_isListing = false;
        if (job->error()) {
            // The last list stays.
            setProblem(job->errorText());
        } else {
            setProblem(QString());
            updateRows(Commands::sortedById(job->workboxes()));
        }
        if (m_shouldListAgain) {
            m_shouldListAgain = false;
            list();
        }
    });
    job->start();
}

void WorkboxModel::updateRows(const QList<Workbox> &workboxes)
{
    int row = 0;
    for (const Workbox &workbox : workboxes) {
        while (row < m_rows.size() && m_rows[row].workbox.id < workbox.id) {
            beginRemoveRows(QModelIndex(), row, row);
            m_rows.removeAt(row);
            endRemoveRows();
        }
        if (row < m_rows.size() && m_rows[row].workbox.id == workbox.id) {
            const Workbox &shown = m_rows[row].workbox;
            if (shown.name != workbox.name || shown.color != workbox.color || shown.isRunning != workbox.isRunning) {
                m_rows[row].workbox = workbox;
                emitRowChanged(row);
            }
        } else {
            beginInsertRows(QModelIndex(), row, row);
            m_rows.insert(row, {.workbox = workbox});
            endInsertRows();
        }
        ++row;
    }
    if (row < m_rows.size()) {
        beginRemoveRows(QModelIndex(), row, static_cast<int>(m_rows.size()) - 1);
        m_rows.resize(row);
        endRemoveRows();
    }
    updateRunningCount();
}

int WorkboxModel::rowOf(const QString &id) const
{
    const auto found = std::ranges::find_if(m_rows, [&id](const Row &row) {
        return row.workbox.id == id;
    });
    return found == m_rows.end() ? -1 : static_cast<int>(found - m_rows.begin());
}

void WorkboxModel::beginOperation(const QString &id, Operation operation, EngineJob *job)
{
    const int row = rowOf(id);
    if (row < 0 || m_rows[row].operation != Operation::None) {
        delete job;
        return;
    }
    Row &changed = m_rows[row];
    changed.operation = operation;
    changed.failedOperation = Operation::None;
    changed.errorText.clear();
    emitRowChanged(row);
    connect(job, &KJob::result, this, [this, id, operation, job] {
        finishOperation(id, operation, job);
    });
    job->start();
}

// Its new state at once, not after the engine's events: the spinner gives way
// to the right button.
void WorkboxModel::finishOperation(const QString &id, Operation operation, const KJob *job)
{
    const int row = rowOf(id);
    if (row < 0) {
        return;
    }
    if (!job->error() && operation == Operation::Removing) {
        beginRemoveRows(QModelIndex(), row, row);
        m_rows.removeAt(row);
        endRemoveRows();
        updateRunningCount();
        return;
    }
    Row &changed = m_rows[row];
    changed.operation = Operation::None;
    if (job->error()) {
        changed.failedOperation = operation;
        changed.errorText = job->errorText();
    } else {
        changed.workbox.isRunning = operation == Operation::Starting;
    }
    emitRowChanged(row);
    updateRunningCount();
}

void WorkboxModel::emitRowChanged(int row)
{
    const QModelIndex changed = index(row);
    Q_EMIT dataChanged(changed, changed);
}

void WorkboxModel::updateRunningCount()
{
    const int runningCount = static_cast<int>(std::ranges::count_if(m_rows, [](const Row &row) {
        return row.workbox.isRunning;
    }));
    if (runningCount != m_runningCount) {
        m_runningCount = runningCount;
        Q_EMIT runningCountChanged();
    }
}

void WorkboxModel::setProblem(const QString &problem)
{
    if (problem != m_problem) {
        m_problem = problem;
        Q_EMIT problemChanged();
    }
}
