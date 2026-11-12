// KRunner plugin for BrescaOS (README.md): "bresca updates check", or its
// start, checks for updates in System Settings > BrescaOS Updates.
#include <KLocalizedString>
#include <KPluginFactory>
#include <KRunner/AbstractRunner>
#include <KRunner/QueryMatch>
#include <KRunner/RunnerContext>

#include <QProcess>

namespace
{
const QString checkCommand = QStringLiteral("bresca updates check");

// Starts the check, as the tray applet's Check for Updates does, and opens
// System Settings > BrescaOS Updates, which shows it running and its result.
// An administrator at the computer needs no password (bresca-updates.rules);
// anyone else gets polkit's dialog.
void checkInSettings()
{
    QProcess::startDetached(QStringLiteral("systemctl"), {QStringLiteral("start"), QStringLiteral("--no-block"), QStringLiteral("bresca-updates@check.service")});
    QProcess::startDetached(QStringLiteral("systemsettings"), {QStringLiteral("kcm_brescaos_updates")});
}
}

class BrescaRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    BrescaRunner(QObject *parent, const KPluginMetaData &metaData)
        : KRunner::AbstractRunner(parent, metaData)
    {
        addSyntax(checkCommand, i18n("Checks whether a newer BrescaOS version is available"));
    }

    // The command, or its start, in any case and with any spaces. Its
    // results come first: it's KRunner's first favorite (krunnerrc).
    void match(KRunner::RunnerContext &context) override
    {
        const QString query = context.query().simplified().toLower();
        if (query.isEmpty() || !checkCommand.startsWith(query)) {
            return;
        }

        KRunner::QueryMatch match(this);
        match.setId(checkCommand);
        match.setText(i18n("Check for BrescaOS updates"));
        match.setSubtext(checkCommand);
        match.setIconName(QStringLiteral("system-software-update"));
        match.setCategoryRelevance(KRunner::QueryMatch::CategoryRelevance::Highest);
        match.setRelevance(query == checkCommand ? 1.0 : 0.9);
        context.addMatch(match);
    }

    void run(const KRunner::RunnerContext &, const KRunner::QueryMatch &) override
    {
        checkInSettings();
    }
};

K_PLUGIN_CLASS_WITH_JSON(BrescaRunner, "krunner_brescaos.json")

#include "runner.moc"
