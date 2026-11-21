// workbox <command> [options] (README.md): reads each command's arguments,
// runs it on the engine workbox.conf names and prints its result.
#include "commands.h"
#include "podmanengine.h"
#include "settings.h"

#include <KLocalizedString>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QTranslator>

#include <algorithm>
#include <memory>

namespace
{

// The image's settings; the user's are in ~/.config/workbox/workbox.conf.
const QString systemSettingsFile = QStringLiteral("/usr/share/workbox/workbox.conf");

void printLine(FILE *stream, const QString &line)
{
    QTextStream(stream) << line << Qt::endl;
}

Settings readConfiguredSettings()
{
    return readSettings(systemSettingsFile,
                        QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/workbox/workbox.conf"));
}

std::unique_ptr<Engine> engineFor(const Settings &settings)
{
    if (settings.engine != QLatin1String("podman")) {
        throw WorkboxError{i18n("unknown engine \"%1\" in workbox.conf: only podman", settings.engine)};
    }
    return std::make_unique<PodmanEngine>();
}

// The id that command takes, from its arguments (behind "workbox <command>");
// exits after its help with --help, or after the error with a wrong option.
QString parseId(QCommandLineParser &parser, const QString &command, const QStringList &arguments)
{
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("id"), i18n("The workbox's id."), command + QStringLiteral(" <id>"));
    parser.process(QStringList{arguments.first()} + arguments.mid(2));
    if (parser.positionalArguments().size() != 1) {
        throw WorkboxError{i18n("workbox %1 takes one id (workbox %1 --help)", command)};
    }
    return parser.positionalArguments().first();
}

int create(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        i18n("Creates a workbox: a container that shares nothing with the host.\n"
             "\n"
             "Example:\n"
             "  workbox create c1 --name \"Client 01\" --color '#3584e4'"));
    const QCommandLineOption nameOption(QStringLiteral("name"), i18n("The name it's shown with (default: its id)."), i18nc("value of --name", "name"));
    const QCommandLineOption colorOption(QStringLiteral("color"),
                                         i18n("Its color: #rgb, #rrggbb or r,g,b, each 0-255 (default: the palette's color "
                                              "that looks the most different from the other workboxes')."),
                                         i18nc("value of --color", "color"));
    parser.addOptions({nameOption, colorOption});
    const QString id = parseId(parser, QStringLiteral("create"), arguments);
    const auto optionValue = [&parser](const QCommandLineOption &option) {
        return parser.isSet(option) ? std::optional(parser.value(option)) : std::nullopt;
    };

    const Settings settings = readConfiguredSettings();
    const Workbox workbox = Commands::create(*engineFor(settings), id, optionValue(nameOption), optionValue(colorOption), settings.image);
    printLine(stdout, i18n("Created the workbox %1 (\"%2\").", workbox.id, workbox.name));
    return 0;
}

bool confirmRemoval(const Workbox &workbox)
{
    QTextStream(stdout) << i18n("Remove the workbox %1 (\"%2\") and its home? Everything in them will be lost. [y/N] ", workbox.id, workbox.name)
                        << Qt::flush;
    const QString answer = QTextStream(stdin).readLine().trimmed();
    return answer.compare(i18nc("the answer to [y/N] that removes it", "y"), Qt::CaseInsensitive) == 0;
}

int remove(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        i18n("Removes a workbox and its home, stopping it first if it runs: everything in them is lost. "
             "Asks before doing it, unless --yes.\n"
             "\n"
             "Example:\n"
             "  workbox rm c1"));
    const QCommandLineOption yesOption({QStringLiteral("y"), QStringLiteral("yes")}, i18n("Don't ask before removing it."));
    parser.addOption(yesOption);
    const QString id = parseId(parser, QStringLiteral("rm"), arguments);

    const std::unique_ptr<Engine> engine = engineFor(readConfiguredSettings());
    const Workbox workbox = Commands::find(*engine, id);
    if (!parser.isSet(yesOption) && !confirmRemoval(workbox)) {
        printLine(stdout, i18n("Nothing removed."));
        return 1;
    }
    engine->remove(id);
    printLine(stdout, i18n("Removed the workbox %1.", id));
    return 0;
}

int list(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        i18n("Lists the workboxes, by id, with their names and colors.\n"
             "\n"
             "Example:\n"
             "  workbox list"));
    const QCommandLineOption quietOption({QStringLiteral("q"), QStringLiteral("quiet")}, i18n("Only their ids, one per line."));
    parser.addOption(quietOption);
    parser.addHelpOption();
    parser.process(QStringList{arguments.first()} + arguments.mid(2));
    if (!parser.positionalArguments().isEmpty()) {
        throw WorkboxError{i18n("workbox list takes no arguments (workbox list --help)")};
    }

    const QList<Workbox> workboxes = Commands::list(*engineFor(readConfiguredSettings()));
    if (parser.isSet(quietOption)) {
        for (const Workbox &workbox : workboxes) {
            printLine(stdout, workbox.id);
        }
        return 0;
    }
    if (workboxes.isEmpty()) {
        printLine(stdout, i18n("No workboxes yet: workbox create <id> creates one."));
        return 0;
    }

    const QString idHeader = i18nc("column of workbox list", "ID");
    const QString nameHeader = i18nc("column of workbox list", "NAME");
    const QString colorHeader = i18nc("column of workbox list", "COLOR");
    qsizetype idWidth = idHeader.size();
    qsizetype nameWidth = nameHeader.size();
    for (const Workbox &workbox : workboxes) {
        idWidth = std::max(idWidth, workbox.id.size());
        nameWidth = std::max(nameWidth, workbox.name.size());
    }
    const auto printRow = [idWidth, nameWidth](const QString &id, const QString &name, const QString &color) {
        printLine(stdout, id.leftJustified(idWidth) + QStringLiteral("  ") + name.leftJustified(nameWidth) + QStringLiteral("  ") + color);
    };
    printRow(idHeader, nameHeader, colorHeader);
    for (const Workbox &workbox : workboxes) {
        printRow(workbox.id, workbox.name, workbox.color.name());
    }
    return 0;
}

}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("workbox"));
    // QCommandLineParser's own words in its help and errors.
    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale(), QStringLiteral("qtbase"), QStringLiteral("_"), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        QCoreApplication::installTranslator(&qtTranslator);
    }

    const QStringList arguments = QCoreApplication::arguments();
    const QString command = arguments.value(1);
    try {
        if (command == QLatin1String("create")) {
            return create(arguments);
        }
        if (command == QLatin1String("rm")) {
            return remove(arguments);
        }
        if (command == QLatin1String("list")) {
            return list(arguments);
        }
        if (command.isEmpty() || command == QLatin1String("--help") || command == QLatin1String("-h")) {
            printLine(stdout,
                      i18n("Usage: workbox <command> [options]\n"
                           "\n"
                           "Runs containers, each a separate place to work, that share nothing with the host.\n"
                           "\n"
                           "Commands:\n"
                           "  create  Create a workbox\n"
                           "  list    List the workboxes\n"
                           "  rm      Remove a workbox and its home\n"
                           "\n"
                           "\"workbox <command> --help\" shows a command's options."));
            return command.isEmpty() ? 1 : 0;
        }
        throw WorkboxError{i18n("unknown command \"%1\" (workbox --help)", command)};
    } catch (const WorkboxError &error) {
        printLine(stderr, QStringLiteral("workbox: ") + error.message);
        return 1;
    }
}
