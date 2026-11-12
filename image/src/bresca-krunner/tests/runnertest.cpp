// What the plugin offers for each query. What choosing it does only runs in
// the development VM: it needs systemd, polkit and System Settings.
#include <KRunner/AbstractRunnerTest>
#include <KRunner/QueryMatch>

#include <QTest>

using namespace KRunner;

class RunnerTest : public AbstractRunnerTest
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        initProperties();
    }

    void testFindsTheCommands_data()
    {
        QTest::addColumn<QString>("query");
        QTest::addColumn<QStringList>("texts");
        const QString check = QStringLiteral("Check for BrescaOS updates");
        const QString install = QStringLiteral("Install the new BrescaOS version");
        QTest::newRow("check, whole") << QStringLiteral("bresca updates check") << QStringList{check};
        QTest::newRow("check, in capitals, with extra spaces") << QStringLiteral("  Bresca  UPDATES check ") << QStringList{check};
        QTest::newRow("check, being typed") << QStringLiteral("bresca updates ch") << QStringList{check};
        QTest::newRow("install, whole") << QStringLiteral("bresca updates install") << QStringList{install};
        QTest::newRow("install, being typed") << QStringLiteral("bresca updates inst") << QStringList{install};
        QTest::newRow("both, their common start") << QStringLiteral("bresca upd") << QStringList{check, install};
        QTest::newRow("both, the start of their first word") << QStringLiteral("bre") << QStringList{check, install};
    }

    void testFindsTheCommands()
    {
        QFETCH(QString, query);
        QFETCH(QStringList, texts);
        QStringList foundTexts;
        for (const QueryMatch &match : launchQuery(query)) {
            foundTexts << match.text();
        }
        foundTexts.sort();
        QCOMPARE(foundTexts, texts);
    }

    // With only this plugin (Ctrl+Alt+B).
    void testFindsTheCommandsWithOnlyThePlugin_data()
    {
        QTest::addColumn<QString>("query");
        QTest::addColumn<QStringList>("texts");
        const QString check = QStringLiteral("Check for BrescaOS updates");
        const QString install = QStringLiteral("Install the new BrescaOS version");
        QTest::newRow("KRunner's help") << QStringLiteral("?") << QStringList{check, install};
        QTest::newRow("without bresca") << QStringLiteral("updates") << QStringList{check, install};
        QTest::newRow("without bresca, being typed") << QStringLiteral("updates ch") << QStringList{check};
        QTest::newRow("with bresca") << QStringLiteral("bresca updates install") << QStringList{install};
        QTest::newRow("another command")
            << QStringLiteral("firefox") << QStringList{QStringLiteral("“firefox” isn't a BrescaOS command: search all of KRunner")};
    }

    void testFindsTheCommandsWithOnlyThePlugin()
    {
        QFETCH(QString, query);
        QFETCH(QStringList, texts);
        QStringList foundTexts;
        for (const QueryMatch &match : launchQuery(query, QStringLiteral("krunner_brescaos"))) {
            foundTexts << match.text();
        }
        foundTexts.sort();
        QCOMPARE(foundTexts, texts);
    }

    void testFindsNothing_data()
    {
        QTest::addColumn<QString>("query");
        QTest::newRow("another word") << QStringLiteral("firefox");
        QTest::newRow("one of its words, not first") << QStringLiteral("updates");
        QTest::newRow("its words in another order") << QStringLiteral("bresca check");
        QTest::newRow("more than a command") << QStringLiteral("bresca updates check now");
    }

    void testFindsNothing()
    {
        QFETCH(QString, query);
        QVERIFY(launchQuery(query).isEmpty());
    }
};

QTEST_MAIN(RunnerTest)

#include "runnertest.moc"
