// bresca-distrobox-colors <distrobox enter's arguments>: prints, one per
// line, the podman exec flags that give the distrobox it enters its colors
// (README.md). distrobox.conf runs it in distrobox-enter.
#include "terminalcolors.h"

#include <KColorScheme>
#include <QGuiApplication>

#include <cstdio>

int main(int argc, char **argv)
{
    // Read before QGuiApplication, which takes the Qt options out of argv.
    QStringList arguments;
    for (int i = 1; i < argc; ++i) {
        arguments << QString::fromLocal8Bit(argv[i]);
    }

    // KColorScheme needs an application; offscreen opens no window and needs
    // no display.
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication application(argc, argv);

    using namespace TerminalColors;
    const QString name = enteredName(arguments, qEnvironmentVariable("DBX_CONTAINER_NAME"));
    // Konsole's accent: its palette's highlight, which Plasma takes from the
    // color scheme's selection background.
    const QColor accent = KColorScheme(QPalette::Active, KColorScheme::Selection).background().color();
    const Colors colors = forHue(oklchHue(konsoleTabColor(name, accent)));
    std::printf("--env=BRESCAOS_DISTROBOX_COLOR=%s\n--env=BRESCAOS_DISTROBOX_LABEL_COLOR=%s\n",
                qPrintable(colors.background.name()),
                qPrintable(colors.label.name()));
    return 0;
}
