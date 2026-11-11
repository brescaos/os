// Bresca Welcome: first steps on BrescaOS (README.md).
//
//   bresca-welcome           every page (application menu); records nothing
//   bresca-welcome --login   the same at login (autostart), but only when
//                            Revision::current is newer than what the user
//                            was last shown

#include "revision.h"

#include <KLocalizedQmlContext>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QSysInfo>
#include <QtQml/qqmlextensionplugin.h>

// The QML module is a static library, shared with the tests.
Q_IMPORT_QML_PLUGIN(com_brescaos_welcomePlugin)

int main(int argc, char *argv[])
{
    // QApplication, not QGuiApplication: Plasma's platform theme then gives the
    // window the user's colour scheme and follows it when it changes.
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain(TRANSLATION_DOMAIN);
    // The BrescaOS version (VERSION_ID in /usr/lib/os-release), for --version
    // and the welcome page's title.
    QApplication::setApplicationVersion(QSysInfo::productVersion());
    QApplication::setDesktopFileName(QStringLiteral("com.brescaos.welcome"));
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("help-about")));

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("First steps on BrescaOS"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption loginOption(QStringLiteral("login"),
                                         i18n("Open only if this version of BrescaOS has a welcome the user hasn't been shown (at login)."));
    parser.addOption(loginOption);
    parser.process(app);

    if (parser.isSet(loginOption)) {
        if (Revision::readShown() >= Revision::current) {
            return 0;
        }
        // Recorded now, so it opens once even if it's closed early.
        Revision::writeShown(Revision::current);
    }

    if (QQuickStyle::name().isEmpty()) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.loadFromModule("com.brescaos.welcome", "Main");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    return app.exec();
}
