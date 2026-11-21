#include "mainwindow.h"
#include "trayicon.h"

#include <KLocalizedString>

#include <QApplication>
#include <QIcon>

int main(int argc, char **argv)
{
    QApplication application(argc, argv);
    KLocalizedString::setApplicationDomain("workbox");
    application.setApplicationDisplayName(i18nc("@title the app's name", "Workbox"));
    QApplication::setDesktopFileName(QStringLiteral("com.brescaos.workbox"));
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("com.brescaos.workbox")));

    // The list the window and the tray icon share; the icon goes with the
    // app, when its window closes.
    WorkboxModel model;
    MainWindow window(model);
    TrayIcon trayIcon(model, window);
    window.show();
    return application.exec();
}
