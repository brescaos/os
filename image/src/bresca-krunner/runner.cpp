// KRunner plugin for BrescaOS (README.md): the bresca commands below, or
// their start, open System Settings > BrescaOS Updates, which does them.
#include <KLazyLocalizedString>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KRunner/AbstractRunner>
#include <KRunner/QueryMatch>
#include <KRunner/RunnerContext>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QProcess>

#include <array>

namespace
{
struct Command {
    QString command;
    KLazyLocalizedString text;
    KLazyLocalizedString description;
    // What the module does (--args): check, or install, which also downloads
    // and installs a new version. None: it only opens.
    QString moduleAction;
};

const QString commandPrefix = QStringLiteral("bresca ");

// "buc" for "bresca updates check".
QString initialsOf(const QString &command)
{
    QString initials;
    for (const QString &word : command.split(u' ')) {
        initials += word.front();
    }
    return initials;
}

// The start of the command ("bresca upd") or of its initials ("bu").
bool isStartOf(const QString &typed, const QString &command)
{
    return command.startsWith(typed) || initialsOf(command).startsWith(typed);
}

bool isWhole(const QString &typed, const QString &command)
{
    return typed == command || typed == initialsOf(command);
}

const std::array commands{
    Command{QStringLiteral("bresca updates"), kli18n("Open BrescaOS Updates"), kli18n("Opens System Settings > BrescaOS Updates"), QString()},
    Command{QStringLiteral("bresca updates check"), kli18n("Check for BrescaOS updates"),
            kli18n("Checks whether a newer BrescaOS version is available"), QStringLiteral("check")},
    Command{QStringLiteral("bresca updates install"), kli18n("Install the new BrescaOS version"),
            kli18n("Downloads and installs a newer BrescaOS version, if there's one"), QStringLiteral("install")},
};

// What run() does with a match: its data.
const QString openSettingsKind = QStringLiteral("open-settings");
const QString searchEverywhereKind = QStringLiteral("search-everywhere");

// The module asks first for an administrator's password (polkit), and
// before downloading from a registry without TLS.
void openUpdatesSettings(const QString &moduleAction)
{
    QStringList arguments{QStringLiteral("kcm_brescaos_updates")};
    if (!moduleAction.isEmpty()) {
        arguments << QStringLiteral("--args") << moduleAction;
    }
    QProcess::startDetached(QStringLiteral("systemsettings"), arguments);
}

// KRunner again, with every plugin and query typed.
void searchEverywhere(const QString &query)
{
    QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("org.kde.krunner"), QStringLiteral("/App"), QStringLiteral("org.kde.krunner.App"),
                                                       QStringLiteral("query"));
    call << query;
    QDBusConnection::sessionBus().send(call);
}
}

class BrescaRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    BrescaRunner(QObject *parent, const KPluginMetaData &metaData)
        : KRunner::AbstractRunner(parent, metaData)
    {
        for (const Command &command : commands) {
            addSyntax(command.command, command.description.toString());
        }
    }

    // Each command the query is the start of (isStartOf()), in any case. Its
    // results come first: it's KRunner's first favorite. With only 
    // this plugin (Ctrl+Alt+B, com.brescaos.bresca-krunner.desktop), 
    // also without "bresca ", every
    // command for "?" (KRunner's help button types it, and its help plugin
    // isn't there), and for anything else a way out to the whole KRunner.
    // KRunner never asks for an empty query.
    void match(KRunner::RunnerContext &context) override
    {
        const bool isOnlyPlugin = context.singleRunnerQueryMode();
        const QString query = context.query().simplified().toLower();
        if (query.isEmpty()) {
            return;
        }
        const bool isHelp = isOnlyPlugin && query == QLatin1String("?");
        bool isCommandFound = false;
        for (const Command &command : commands) {
            // What can be typed of it: without "bresca " too with only this plugin.
            const QString shortCommand = command.command.mid(commandPrefix.size());
            if (!isHelp && !isStartOf(query, command.command) && !(isOnlyPlugin && isStartOf(query, shortCommand))) {
                continue;
            }
            // Ahead of the longer commands it's the start of.
            const bool isWholeCommand = isWhole(query, command.command) || (isOnlyPlugin && isWhole(query, shortCommand));
            isCommandFound = true;
            KRunner::QueryMatch match(this);
            match.setId(command.command);
            match.setText(command.text.toString());
            match.setIconName(QStringLiteral("com.brescaos.updates"));
            match.setData(QStringList{openSettingsKind, command.moduleAction});
            match.setCategoryRelevance(KRunner::QueryMatch::CategoryRelevance::Highest);
            match.setRelevance(isWholeCommand ? 1.0 : 0.9);
            context.addMatch(match);
        }
        if (isOnlyPlugin && !isCommandFound) {
            const QString typed = context.query().trimmed();
            KRunner::QueryMatch match(this);
            match.setId(searchEverywhereKind);
            match.setText(i18n("“%1” isn't a BrescaOS command: search all of KRunner", typed));
            match.setIconName(QStringLiteral("system-search"));
            match.setData(QStringList{searchEverywhereKind, typed});
            match.setCategoryRelevance(KRunner::QueryMatch::CategoryRelevance::Low);
            context.addMatch(match);
        }
    }

    void run(const KRunner::RunnerContext &, const KRunner::QueryMatch &match) override
    {
        const QStringList action = match.data().toStringList();
        if (action.first() == openSettingsKind) {
            openUpdatesSettings(action.at(1));
        } else {
            searchEverywhere(action.at(1));
        }
    }
};

K_PLUGIN_CLASS_WITH_JSON(BrescaRunner, "krunner_brescaos.json")

#include "runner.moc"
