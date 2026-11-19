// KRunner plugin for Distrobox (README.md): creates, stops and removes
// distroboxes, showing their progress and result in Plasma's job
// notification.
#include "distroboxcommands.h"
#include "distroboxjob.h"

#include <KLocalizedString>
#include <KMessageDialog>
#include <KPluginFactory>
#include <KRunner/AbstractRunner>
#include <KRunner/QueryMatch>
#include <KRunner/RunnerContext>
#include <KStandardGuiItem>
#include <KUiServerV2JobTracker>

#include <QCoreApplication>
#include <QIcon>
#include <QProcess>
#include <QRegularExpression>

#include <algorithm>
#include <optional>

namespace
{

// Distrobox's icon and its hidden .desktop file (the progress notifications'
// application).
const QString distroboxId = QStringLiteral("com.brescaos.distrobox");

enum class Action {
    Create,
    Stop,
    Remove,
};

struct Query {
    Action action;
    // The whole name to create, or the start of the one to stop or remove.
    QString name;
};

// "distrobox create|stop|rm <name>", or its initials: "dc|ds|dr <name>".
std::optional<Query> parseQuery(const QString &query)
{
    static const QRegularExpression syntax(QStringLiteral(R"(^\s*(?:distrobox\s+(create|stop|rm)|(dc|ds|dr))\s+(\S+)\s*$)"),
                                           QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch typed = syntax.match(query);
    if (!typed.hasMatch()) {
        return std::nullopt;
    }
    const QString command = (typed.captured(1) + typed.captured(2)).toLower();
    const Action action = command == QLatin1String("create") || command == QLatin1String("dc") ? Action::Create
        : command == QLatin1String("stop") || command == QLatin1String("ds")                    ? Action::Stop
                                                                                                : Action::Remove;
    return Query{action, typed.captured(3)};
}

// The names podman accepts.
bool isValidName(const QString &name)
{
    static const QRegularExpression validName(QStringLiteral("^[a-zA-Z0-9][a-zA-Z0-9_.-]*$"));
    return validName.match(name).hasMatch();
}

QList<DistroboxCommands::ListedDistrobox> listDistroboxes()
{
    QProcess podman;
    podman.start(QStringLiteral("podman"), DistroboxCommands::listArguments());
    if (!podman.waitForFinished() || podman.exitStatus() != QProcess::NormalExit || podman.exitCode() != 0) {
        qWarning("podman ps failed: %s", qPrintable(QString::fromUtf8(podman.readAllStandardError()).trimmed() + podman.errorString()));
        return {};
    }
    return DistroboxCommands::parseListing(QString::fromUtf8(podman.readAllStandardOutput()));
}

// In Plasma's job notification, titled title: while it runs, then Plasma
// adds "(Finished)" to it, or "(Failed)" with what distrobox said.
void runWithProgress(DistroboxCommands::Command command, const QString &name, const QString &title)
{
    static auto *tracker = new KUiServerV2JobTracker(QCoreApplication::instance());
    auto *job = new DistroboxJob(command, name, title);
    job->setProperty("desktopFileName", distroboxId);
    // Shown at once, not after a few seconds.
    job->setProperty("immediateProgressReporting", true);
    tracker->registerJob(job);
    job->start();
}

void create(const QString &name)
{
    runWithProgress(&DistroboxCommands::create, name, i18n("Creating distrobox “%1”", name));
}

void stop(const QString &name)
{
    runWithProgress(&DistroboxCommands::stop, name, i18n("Stopping distrobox “%1”", name));
}

// After asking, in a dialog that doesn't block the process KRunner runs in
// (plasmashell for the launcher's search).
void confirmRemoving(const QString &name)
{
    auto *dialog = new KMessageDialog(
        KMessageDialog::WarningContinueCancel,
        i18n("Remove the distrobox “%1”? Everything installed in it will be lost. Your home folder is not affected.", name));
    dialog->setCaption(i18n("Remove distrobox “%1”", name));
    dialog->setWindowIcon(QIcon::fromTheme(distroboxId));
    dialog->setButtons(KGuiItem(i18n("Remove"), QStringLiteral("edit-delete")), KGuiItem(), KStandardGuiItem::cancel());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(dialog, &QDialog::finished, dialog, [name](int result) {
        if (result == KMessageDialog::PrimaryAction) {
            runWithProgress(&DistroboxCommands::remove, name, i18n("Removing distrobox “%1”", name));
        }
    });
    dialog->show();
}

}

class DistroboxRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    DistroboxRunner(QObject *parent, const KPluginMetaData &metaData)
        : KRunner::AbstractRunner(parent, metaData)
    {
        addSyntax(QStringList{QStringLiteral("distrobox create :q:"), QStringLiteral("dc :q:")}, i18n("Creates the distrobox :q:"));
        addSyntax(QStringList{QStringLiteral("distrobox stop :q:"), QStringLiteral("ds :q:")}, i18n("Stops the running distrobox :q:"));
        addSyntax(QStringList{QStringLiteral("distrobox rm :q:"), QStringLiteral("dr :q:")}, i18n("Removes the distrobox :q:"));
        // Once per search, in this runner's thread, before its first match().
        connect(this, &KRunner::AbstractRunner::prepare, this, [this] {
            m_distroboxes = listDistroboxes();
        });
    }

    // Create: the name typed, if it can be a new distrobox's; if not, why, in
    // a result that can't be chosen. Stop and remove: each distrobox (stop:
    // each running one) whose name starts with what's typed; the one with
    // exactly that name first.
    void match(KRunner::RunnerContext &context) override
    {
        const std::optional<Query> query = parseQuery(context.query());
        if (!query) {
            return;
        }
        if (query->action == Action::Create) {
            addCreateMatch(context, query->name);
            return;
        }
        for (const DistroboxCommands::ListedDistrobox &distrobox : std::as_const(m_distroboxes)) {
            if (!distrobox.name.startsWith(query->name) || (query->action == Action::Stop && !distrobox.isRunning)) {
                continue;
            }
            const QString text = query->action == Action::Stop ? i18n("Stop distrobox “%1”", distrobox.name) : i18n("Remove distrobox “%1”", distrobox.name);
            KRunner::QueryMatch match = newMatch(text, query->action, distrobox.name);
            match.setRelevance(distrobox.name == query->name ? 1.0 : 0.9);
            context.addMatch(match);
        }
    }

    // In the GUI thread: KRunner runs this in the runner's own, and the
    // dialog and the progress notifications belong to the GUI's.
    void run(const KRunner::RunnerContext &, const KRunner::QueryMatch &match) override
    {
        const QVariantList data = match.data().toList();
        const auto action = static_cast<Action>(data.at(0).toInt());
        const QString name = data.at(1).toString();
        QMetaObject::invokeMethod(QCoreApplication::instance(), [action, name] {
            switch (action) {
            case Action::Create:
                create(name);
                break;
            case Action::Stop:
                stop(name);
                break;
            case Action::Remove:
                confirmRemoving(name);
                break;
            }
        });
    }

private:
    void addCreateMatch(KRunner::RunnerContext &context, const QString &name)
    {
        const bool exists = std::ranges::any_of(m_distroboxes, [&name](const DistroboxCommands::ListedDistrobox &distrobox) {
            return distrobox.name == name;
        });
        if (!isValidName(name)) {
            KRunner::QueryMatch match = newMatch(i18n("“%1” can't be a distrobox's name", name), Action::Create, name);
            match.setSubtext(i18n("Use only letters, digits, “_”, “.” and “-”, starting with a letter or a digit"));
            match.setEnabled(false);
            context.addMatch(match);
        } else if (exists) {
            KRunner::QueryMatch match = newMatch(i18n("Distrobox “%1” already exists", name), Action::Create, name);
            match.setEnabled(false);
            context.addMatch(match);
        } else {
            context.addMatch(newMatch(i18n("Create distrobox “%1”", name), Action::Create, name));
        }
    }

    // The query starts with a Distrobox command: its results come first.
    KRunner::QueryMatch newMatch(const QString &text, Action action, const QString &name)
    {
        KRunner::QueryMatch match(this);
        match.setId(QStringLiteral("%1:%2").arg(static_cast<int>(action)).arg(name));
        match.setText(text);
        match.setIconName(distroboxId);
        match.setData(QVariantList{static_cast<int>(action), name});
        match.setCategoryRelevance(KRunner::QueryMatch::CategoryRelevance::Highest);
        match.setRelevance(1.0);
        return match;
    }

    // podman's listing when the search started.
    QList<DistroboxCommands::ListedDistrobox> m_distroboxes;
};

K_PLUGIN_CLASS_WITH_JSON(DistroboxRunner, "krunner_distrobox.json")

#include "runner.moc"
