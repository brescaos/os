// Runs qml/tst_*.qml with what they compare the script against: its source, as
// the build puts it together, and AutotileLayout's layouts and default gap.
#include "autotilelayout.h"

#include <QFile>
#include <QQmlContext>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QtQuickTest>

using namespace Qt::StringLiterals;

class Setup : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        QFile script(QStringLiteral(SCRIPT_FILE));
        if (!script.open(QIODevice::ReadOnly)) {
            qFatal("can't read %s", SCRIPT_FILE);
        }
        // A kwinrc of its own: AutotileLayout's defaults.
        QStandardPaths::setTestModeEnabled(true);
        QFile::remove(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/kwinrc"_s);
        const AutotileLayout autotile;
        QStringList layoutNames;
        for (const QVariant &layout : autotile.layouts()) {
            layoutNames.append(layout.toMap().value(u"name"_s).toString());
        }
        engine->rootContext()->setContextProperty(u"scriptSource"_s, QString::fromUtf8(script.readAll()));
        engine->rootContext()->setContextProperty(u"autotileLayoutNames"_s, layoutNames);
        engine->rootContext()->setContextProperty(u"autotileDefaultLayout"_s, autotile.layout());
        engine->rootContext()->setContextProperty(u"autotileDefaultGap"_s, autotile.defaultGap());
    }
};

QUICK_TEST_MAIN_WITH_SETUP(scripttest, Setup)

#include "scripttest.moc"
