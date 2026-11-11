// The QML pages (qml/tst_*.qml), offscreen, with the program's QML module
// and i18n.
#include <KLocalizedQmlContext>

#include <QCoreApplication>
#include <QQmlEngine>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQuickTest>

Q_IMPORT_QML_PLUGIN(com_brescaos_welcomePlugin)

class Setup : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void applicationAvailable()
    {
        // As main.cpp sets it from os-release.
        QCoreApplication::setApplicationVersion(QStringLiteral("1.2.3"));
    }

    void qmlEngineAvailable(QQmlEngine *engine)
    {
        KLocalization::setupLocalizedContext(engine);
    }
};

QUICK_TEST_MAIN_WITH_SETUP(welcome, Setup)
#include "qmltest.moc"
