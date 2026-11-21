// workbox <command> [options] (README.md): reads each command's arguments,
// runs it on the engine workbox.conf names and prints its result.
#include "commands.h"
#include "engines.h"
#include "settings.h"
#include "terminalcolors.h"

#include <KLocalizedString>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QLibraryInfo>
#include <QScopeGuard>
#include <QTextStream>
#include <QTranslator>

#include <algorithm>
#include <memory>

#include <unistd.h>

namespace
{

void printLine(FILE *stream, const QString &line)
{
    QTextStream(stream) << line << Qt::endl;
}

// In a terminal with 24-bit color (COLORTERM, as Konsole sets it) and
// without NO_COLOR (no-color.org): list and info show a workbox's id in its
// color, and its status in the terminal's green or red.
bool isColoredTerminal()
{
    const QString colorTerm = qEnvironmentVariable("COLORTERM");
    return isatty(STDOUT_FILENO) && qEnvironmentVariableIsEmpty("NO_COLOR")
        && (colorTerm == QLatin1String("truecolor") || colorTerm == QLatin1String("24bit"));
}

QString paint(const QString &text, const QString &sgrCode)
{
    static const bool isColored = isColoredTerminal();
    return isColored ? QStringLiteral("\x1b[%1m%2\x1b[0m").arg(sgrCode, text) : text;
}

QString paintedId(const Workbox &workbox)
{
    const QColor &color = workbox.color;
    return paint(workbox.id, QStringLiteral("38;2;%1;%2;%3").arg(color.red()).arg(color.green()).arg(color.blue()));
}

QString paintedStatus(const Workbox &workbox)
{
    return workbox.isRunning ? paint(i18nc("status in workbox list", "running"), QStringLiteral("32"))
                             : paint(i18nc("status in workbox list", "stopped"), QStringLiteral("31"));
}

Settings readConfiguredSettings()
{
    return readSettings(settingsFiles());
}

// Runs job to its end, its output on stderr as it comes (on a terminal, as the
// engine writes it: Podman's progress bars), and gives it back; throws its
// error. Waiting is fine here: this is the command line.
template<typename Job>
std::unique_ptr<Job> runToEnd(Job *job)
{
    std::unique_ptr<Job> ownedJob(job);
    ownedJob->setAutoDelete(false);
    ownedJob->setOutputToTerminal(isatty(STDERR_FILENO));
    QObject::connect(ownedJob.get(), &EngineJob::output, [](const QString &line) {
        printLine(stderr, line);
    });
    if (!ownedJob->exec()) {
        throw WorkboxError{ownedJob->errorString()};
    }
    return ownedJob;
}

QList<Workbox> listWorkboxes(Engine &engine)
{
    return runToEnd(engine.list())->workboxes();
}

std::unique_ptr<Engine> engineFor(const Settings &settings)
{
    return engineNamed(settings.engine);
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

// The id that command takes or, with allOption (added here), none; the rest
// as parseId().
std::optional<QString>
parseIdOrAll(QCommandLineParser &parser, const QCommandLineOption &allOption, const QString &command, const QStringList &arguments)
{
    parser.addOption(allOption);
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("id"), i18n("The workbox's id."), command + QStringLiteral(" <id>|--all"));
    parser.process(QStringList{arguments.first()} + arguments.mid(2));
    const QStringList ids = parser.positionalArguments();
    if (parser.isSet(allOption)) {
        if (!ids.isEmpty()) {
            throw WorkboxError{i18n("workbox %1 takes an id or --all, not both", command)};
        }
        return std::nullopt;
    }
    if (ids.size() != 1) {
        throw WorkboxError{i18n("workbox %1 takes one id or --all (workbox %1 --help)", command)};
    }
    return ids.first();
}

int create(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        i18n("Creates a workbox: a container that shares nothing with the host but the folders given with -v.\n"
             "\n"
             "Example:\n"
             "  workbox create c1 --name \"Client 01\" --color '#3584e4' -v ~/Clients/01:~/project"));
    const QCommandLineOption nameOption(QStringLiteral("name"), i18n("The name it's shown with (default: its id)."), i18nc("value of --name", "name"));
    const QCommandLineOption colorOption(QStringLiteral("color"),
                                         i18n("Its color: #rgb, #rrggbb or r,g,b, each 0-255 (default: the palette's color "
                                              "that looks the most different from the other workboxes')."),
                                         i18nc("value of --color", "color"));
    const QCommandLineOption sudoOption(QStringLiteral("sudo"),
                                        i18n("Let the user use sudo in it, without a password: whatever runs in it can then be its root."));
    const QCommandLineOption volumeOption({QStringLiteral("v"), QStringLiteral("volume")},
                                          i18n("A volume, as Podman's -v: folder:target shares a folder of your home at target, "
                                               "a folder of the workbox's home (absolute or ~/…); name:target gives the workbox a "
                                               "volume of its own with that name, and target alone an unnamed one, removed with it. "
                                               "Options: ro or rw. Can be given more than once. Not your whole home, nor a folder "
                                               "with keys or what the host runs at login; in ~/.config and ~/.local, only "
                                               "read-only."),
                                          i18nc("value of --volume", "[folder|name:]target[:options]"));
    const QCommandLineOption waylandOption(QStringLiteral("wayland"),
                                           i18n("Let its apps show windows in your Wayland session, with the GPU. Wayland keeps "
                                                "them from reading other apps' windows and keyboard."));
    parser.addOptions({nameOption, colorOption, sudoOption, volumeOption, waylandOption});
    const QString id = parseId(parser, QStringLiteral("create"), arguments);
    const auto optionValue = [&parser](const QCommandLineOption &option) {
        return parser.isSet(option) ? std::optional(parser.value(option)) : std::nullopt;
    };

    const Settings settings = readConfiguredSettings();
    const std::unique_ptr<Engine> engine = engineFor(settings);
    const Workbox workbox =
        runToEnd(new Commands::CreateJob(*engine,
                                         id,
                                         {.name = optionValue(nameOption),
                                          .color = optionValue(colorOption),
                                          .hasSudo = parser.isSet(sudoOption),
                                          .volumes = parser.values(volumeOption),
                                          .hasWayland = parser.isSet(waylandOption)},
                                         settings.image))
            ->workbox();
    printLine(stdout, i18n("Created the workbox %1 (\"%2\").", workbox.id, workbox.name));
    return 0;
}

// The user types the workbox's id, so a removal can't be a reflex "y".
bool confirmRemovingAll(const QList<Workbox> &workboxes)
{
    QStringList ids;
    for (const Workbox &workbox : workboxes) {
        ids.append(workbox.id);
    }
    QTextStream(stdout) << i18np("Remove the workbox %2? Everything in it and in its volumes will be lost. [y/N] ",
                                 "Remove the %1 workboxes (%2)? Everything in them and in their volumes will be lost. [y/N] ",
                                 workboxes.size(),
                                 ids.join(QStringLiteral(", ")))
                        << Qt::flush;
    const QString answer = QTextStream(stdin).readLine().trimmed();
    return answer.compare(i18nc("the answer to [y/N] that removes them", "y"), Qt::CaseInsensitive) == 0;
}

bool confirmRemoval(const Workbox &workbox)
{
    QTextStream(stdout) << i18n("Removing the workbox %1 (\"%2\") loses everything in it and in its home.\nType its id to remove it: ",
                                workbox.id,
                                workbox.name)
                        << Qt::flush;
    return QTextStream(stdin).readLine().trimmed() == workbox.id;
}

int remove(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        i18n("Removes a workbox and its home, stopping it first if it runs: everything in them is lost. "
             "Asks for its id first, or with --all, for a yes, unless --yes.\n"
             "\n"
             "Example:\n"
             "  workbox rm c1"));
    const QCommandLineOption yesOption({QStringLiteral("y"), QStringLiteral("yes")}, i18n("Don't ask before removing it."));
    parser.addOption(yesOption);
    const QCommandLineOption allOption({QStringLiteral("a"), QStringLiteral("all")}, i18n("Every workbox."));
    const std::optional<QString> id = parseIdOrAll(parser, allOption, QStringLiteral("rm"), arguments);

    const std::unique_ptr<Engine> engine = engineFor(readConfiguredSettings());
    if (!id) {
        const QList<Workbox> workboxes = Commands::sortedById(listWorkboxes(*engine));
        if (workboxes.isEmpty()) {
            printLine(stdout, i18n("There are no workboxes to remove."));
            return 0;
        }
        if (!parser.isSet(yesOption) && !confirmRemovingAll(workboxes)) {
            printLine(stdout, i18n("Nothing removed."));
            return 1;
        }
        for (const Workbox &workbox : workboxes) {
            runToEnd(engine->remove(workbox));
            printLine(stdout, i18n("Removed the workbox %1.", workbox.id));
        }
        return 0;
    }
    const Workbox workbox = Commands::find(listWorkboxes(*engine), *id);
    if (!parser.isSet(yesOption) && !confirmRemoval(workbox)) {
        printLine(stdout, i18n("Nothing removed."));
        return 1;
    }
    runToEnd(engine->remove(workbox));
    printLine(stdout, i18n("Removed the workbox %1.", workbox.id));
    return 0;
}

int start(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        i18n("Starts a workbox.\n"
             "\n"
             "Example:\n"
             "  workbox start c1"));
    const QString id = parseId(parser, QStringLiteral("start"), arguments);

    const std::unique_ptr<Engine> engine = engineFor(readConfiguredSettings());
    if (Commands::find(listWorkboxes(*engine), id).isRunning) {
        printLine(stdout, i18n("The workbox %1 is already running.", id));
        return 0;
    }
    runToEnd(engine->start(id));
    printLine(stdout, i18n("Started the workbox %1.", id));
    return 0;
}

int stop(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        i18n("Stops a workbox, or with --all every running one, and everything running in them.\n"
             "\n"
             "Example:\n"
             "  workbox stop c1"));
    const QCommandLineOption allOption({QStringLiteral("a"), QStringLiteral("all")}, i18n("Every running workbox."));
    const std::optional<QString> optionalId = parseIdOrAll(parser, allOption, QStringLiteral("stop"), arguments);

    const std::unique_ptr<Engine> engine = engineFor(readConfiguredSettings());
    if (!optionalId) {
        QList<Workbox> running = Commands::sortedById(listWorkboxes(*engine));
        running.removeIf([](const Workbox &workbox) {
            return !workbox.isRunning;
        });
        if (running.isEmpty()) {
            printLine(stdout, i18n("No workbox is running."));
            return 0;
        }
        for (const Workbox &workbox : running) {
            runToEnd(engine->stop(workbox.id));
            printLine(stdout, i18n("Stopped the workbox %1.", workbox.id));
        }
        return 0;
    }
    const QString &id = *optionalId;
    if (!Commands::find(listWorkboxes(*engine), id).isRunning) {
        printLine(stdout, i18n("The workbox %1 is already stopped.", id));
        return 0;
    }
    runToEnd(engine->stop(id));
    printLine(stdout, i18n("Stopped the workbox %1.", id));
    return 0;
}

int enter(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        i18n("Opens a shell in a workbox, or runs a command in it (after --), starting it first if it's stopped. "
             "In a terminal, a shell's background takes the workbox's hue, and its prompt shows its name.\n"
             "\n"
             "Examples:\n"
             "  workbox enter c1\n"
             "  workbox enter c1 -- make -j4"));
    const QCommandLineOption rootOption(QStringLiteral("root"),
                                        i18n("As the workbox's root, in /root: root only inside it, the user outside it."));
    parser.addOption(rootOption);
    // What follows -- is the command, as it is: none of it is workbox's.
    const qsizetype separator = arguments.indexOf(QStringLiteral("--"), 2);
    const QStringList command = separator < 0 ? QStringList() : arguments.mid(separator + 1);
    const QString id = parseId(parser, QStringLiteral("enter"), separator < 0 ? arguments : arguments.first(separator));
    if (separator >= 0 && command.isEmpty()) {
        throw WorkboxError{i18n("nothing to run after -- (workbox enter --help)")};
    }

    const std::unique_ptr<Engine> engine = engineFor(readConfiguredSettings());
    const Workbox workbox = Commands::find(listWorkboxes(*engine), id);
    if (!workbox.isRunning) {
        runToEnd(engine->start(id));
    }
    if (!command.isEmpty()) {
        return runToEnd(engine->enter(id, command, {}, parser.isSet(rootOption)))->exitStatus();
    }

    // The terminal's background and text (OSC 11 and 10) while the shell
    // runs, and its profile's back after it (OSC 111 and 110; Konsole ignores
    // them without the ";"), whatever ends it.
    const TerminalColors colors = terminalColorsFor(workbox.color);
    const bool isTerminal = isatty(STDOUT_FILENO);
    if (isTerminal) {
        QTextStream(stdout) << QStringLiteral("\x1b]11;%1\x07\x1b]10;%2\x07").arg(colors.background.name(), colors.text.name()) << Qt::flush;
    }
    const auto restoreColors = qScopeGuard([isTerminal] {
        if (isTerminal) {
            QTextStream(stdout) << QStringLiteral("\x1b]111;\x07\x1b]110;\x07") << Qt::flush;
        }
    });

    // "🧰 name [user@id dir]$ ", the name bold in the label's color. The name
    // goes in its own variable: bash expands it in the prompt without reading
    // what's in it as escapes or commands.
    const QString prompt = QStringLiteral("\\[\\e[1;38;2;%1;%2;%3m\\]\U0001F9F0 ${WORKBOX_NAME}\\[\\e[0m\\] [\\u@\\h \\W]\\$ ")
                               .arg(colors.label.red())
                               .arg(colors.label.green())
                               .arg(colors.label.blue());
    return runToEnd(engine->enter(id, {}, {{QStringLiteral("PS1"), prompt}, {QStringLiteral("WORKBOX_NAME"), workbox.name}}, parser.isSet(rootOption)))
        ->exitStatus();
}

int list(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        i18n("Lists the workboxes.\n"
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

    const QList<Workbox> workboxes = Commands::sortedById(listWorkboxes(*engineFor(readConfiguredSettings())));
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
    const QString statusHeader = i18nc("column of workbox list", "STATUS");
    qsizetype idWidth = idHeader.size();
    qsizetype nameWidth = nameHeader.size();
    for (const Workbox &workbox : workboxes) {
        idWidth = std::max(idWidth, workbox.id.size());
        nameWidth = std::max(nameWidth, workbox.name.size());
    }
    const auto row = [idWidth, nameWidth](const QString &id, const QString &name, const QString &status) {
        return id.leftJustified(idWidth) + QStringLiteral("  ") + name.leftJustified(nameWidth) + QStringLiteral("  ") + status;
    };
    printLine(stdout, row(idHeader, nameHeader, statusHeader));
    for (const Workbox &workbox : workboxes) {
        // Padded outside the color's codes, which take no room.
        const QString id = paintedId(workbox) + QString(idWidth - workbox.id.size(), QLatin1Char(' '));
        printLine(stdout, row(id, workbox.name, paintedStatus(workbox)));
    }
    return 0;
}

int info(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        i18n("Shows a workbox: its name, color, status, image and settings.\n"
             "\n"
             "Example:\n"
             "  workbox info c1"));
    const QString id = parseId(parser, QStringLiteral("info"), arguments);
    const Workbox workbox = Commands::find(listWorkboxes(*engineFor(readConfiguredSettings())), id);

    QList<std::pair<QString, QString>> fields{
        {i18nc("field of workbox info", "ID"), paintedId(workbox)},
        {i18nc("field of workbox info", "Name"), workbox.name},
        {i18nc("field of workbox info", "Color"), workbox.color.name()},
        {i18nc("field of workbox info", "Status"), paintedStatus(workbox)},
        {i18nc("field of workbox info", "Image"), workbox.image},
        {i18nc("field of workbox info", "Sudo"), workbox.hasSudo ? i18nc("sudo in workbox info", "yes") : i18nc("sudo in workbox info", "no")},
        {i18nc("field of workbox info", "Wayland"),
         workbox.hasWayland ? i18nc("wayland in workbox info", "yes") : i18nc("wayland in workbox info", "no")},
        {i18nc("field of workbox info", "Home"), volumeName(workbox.id, homeVolume)},
    };
    // One line per volume, the first with the label.
    for (qsizetype index = 0; index < workbox.volumes.size(); ++index) {
        const Volume &volume = workbox.volumes[index];
        const QString folders = !volume.name.isEmpty()
            ? i18nc("a volume of the workbox's own in workbox info", "%1 → %2 (volume)", volume.name, volume.target)
            : volume.source.isEmpty()       ? i18nc("an unnamed volume of the workbox's own in workbox info", "%1 (its own)", volume.target)
            : volume.target == volume.source ? volume.source
                                           : volume.source + QStringLiteral(" → ") + volume.target;
        fields.append({index == 0 ? i18nc("field of workbox info", "Volumes") : QString(),
                       volume.isReadOnly ? i18nc("a volume in workbox info", "%1 (read-only)", folders) : folders});
    }
    qsizetype labelWidth = 0;
    for (const auto &[label, value] : fields) {
        labelWidth = std::max(labelWidth, label.size() + 1);
    }
    for (const auto &[label, value] : fields) {
        printLine(stdout, (label.isEmpty() ? QString() : label + QLatin1Char(':')).leftJustified(labelWidth) + QStringLiteral("  ") + value);
    }
    return 0;
}

}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("workbox"));
    QCoreApplication::setApplicationVersion(QStringLiteral(WORKBOX_VERSION));
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
        if (command == QLatin1String("start")) {
            return start(arguments);
        }
        if (command == QLatin1String("stop")) {
            return stop(arguments);
        }
        if (command == QLatin1String("enter")) {
            return enter(arguments);
        }
        if (command == QLatin1String("info")) {
            return info(arguments);
        }
        if (command == QLatin1String("--version")) {
            printLine(stdout, QStringLiteral("workbox " WORKBOX_VERSION));
            return 0;
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
                           "  info    Show a workbox\n"
                           "  start   Start a workbox\n"
                           "  stop    Stop a workbox\n"
                           "  enter   Open a shell in a workbox\n"
                           "  rm      Remove a workbox and its home\n"
                           "\n"
                           "\"workbox <command> --help\" shows a command's options; \"workbox --version\", its version."));
            return command.isEmpty() ? 1 : 0;
        }
        throw WorkboxError{i18n("unknown command \"%1\" (workbox --help)", command)};
    } catch (const WorkboxError &error) {
        printLine(stderr, QStringLiteral("workbox: ") + error.message);
        return 1;
    }
}
