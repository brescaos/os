#include "autotilelayout.h"

#include <KConfig>
#include <KConfigGroup>

#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

using namespace Qt::StringLiterals;

class AutotileLayoutTest : public QObject
{
    Q_OBJECT

private:
    QString path() const
    {
        return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/kwinrc"_s;
    }

    // As another program does: its own KConfig, in the file.
    void write(const char *group, const char *key, const QString &value)
    {
        KConfig config(path(), KConfig::SimpleConfig);
        config.group(QString::fromLatin1(group)).writeEntry(key, value);
        config.sync();
    }

private Q_SLOTS:
    void init()
    {
        QStandardPaths::setTestModeEnabled(true);
        QFile::remove(path());
        KSharedConfig::openConfig(u"kwinrc"_s)->reparseConfiguration();
    }

    void cleanup()
    {
        QFile::remove(path());
    }

    void defaults()
    {
        const AutotileLayout autotile;
        QCOMPARE(autotile.enabled(), false);
        QCOMPARE(autotile.layout(), u"binaryTree"_s);
    }

    void readsTheFile()
    {
        write("Plugins", "com.brescaos.autotileEnabled", u"true"_s);
        const AutotileLayout autotile;
        QCOMPARE(autotile.enabled(), true);
    }

    void followsTheFile()
    {
        AutotileLayout autotile;
        QSignalSpy enabledChanged(&autotile, &AutotileLayout::enabledChanged);
        write("Plugins", "com.brescaos.autotileEnabled", u"true"_s);
        QTRY_COMPARE(autotile.enabled(), true);
        QCOMPARE(enabledChanged.count(), 1);
    }

    void showInTrayIsOffByDefaultAndSaved()
    {
        AutotileLayout autotile;
        QCOMPARE(autotile.showInTray(), false);
        QSignalSpy showInTrayChanged(&autotile, &AutotileLayout::showInTrayChanged);
        autotile.setShowInTray(true);
        QCOMPARE(autotile.showInTray(), true);
        QCOMPARE(showInTrayChanged.count(), 1);
        const KConfig config(path(), KConfig::SimpleConfig);
        QCOMPARE(config.group(u"Script-com.brescaos.autotile"_s).readEntry("ShowInTray", false), true);
    }

    void followsShowInTrayInTheFile()
    {
        AutotileLayout autotile;
        write("Script-com.brescaos.autotile", "ShowInTray", u"true"_s);
        QTRY_COMPARE(autotile.showInTray(), true);
    }

    void gapHasADefaultAndIsSaved()
    {
        AutotileLayout autotile;
        QCOMPARE(autotile.gap(), autotile.defaultGap());
        QSignalSpy gapChanged(&autotile, &AutotileLayout::gapChanged);
        autotile.setGap(20);
        QCOMPARE(autotile.gap(), 20);
        QCOMPARE(gapChanged.count(), 1);
        const KConfig config(path(), KConfig::SimpleConfig);
        QCOMPARE(config.group(u"Script-com.brescaos.autotile"_s).readEntry("Gap", 0), 20);
    }

    void unknownLayoutIsTheDefault()
    {
        write("Script-com.brescaos.autotile", "Layout", u"kde"_s);
        const AutotileLayout autotile;
        QCOMPARE(autotile.layout(), u"binaryTree"_s);
    }

    void listsTheLayouts()
    {
        const AutotileLayout autotile;
        QCOMPARE(autotile.layouts().size(), 3);
        const QVariantMap first = autotile.layouts().first().toMap();
        QCOMPARE(first.value(u"name"_s), u"binaryTree"_s);
        QCOMPARE(first.value(u"label"_s), u"Binary tree"_s);
    }

    void setEnabledSavesInPlugins()
    {
        AutotileLayout autotile;
        QSignalSpy enabledChanged(&autotile, &AutotileLayout::enabledChanged);
        autotile.setEnabled(true);
        QCOMPARE(autotile.enabled(), true);
        QCOMPARE(enabledChanged.count(), 1);
        const auto saved = [this] {
            const KConfig config(path(), KConfig::SimpleConfig);
            return config.group(u"Plugins"_s).readEntry("com.brescaos.autotileEnabled", false);
        };
        QCOMPARE(saved(), true);
        autotile.setEnabled(false);
        QCOMPARE(autotile.enabled(), false);
        QCOMPARE(saved(), false);
    }

    void setLayoutSavesInTheScriptsGroup()
    {
        AutotileLayout autotile;
        autotile.setLayout(u"binaryTree"_s);
        QCOMPARE(autotile.layout(), u"binaryTree"_s);
        const KConfig config(path(), KConfig::SimpleConfig);
        QCOMPARE(config.group(u"Script-com.brescaos.autotile"_s).readEntry("Layout"), u"binaryTree"_s);
    }
};

QTEST_GUILESS_MAIN(AutotileLayoutTest)

#include "autotilelayouttest.moc"
