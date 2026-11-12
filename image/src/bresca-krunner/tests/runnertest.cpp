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

    void testFindsTheCheck_data()
    {
        QTest::addColumn<QString>("query");
        QTest::newRow("the whole command") << QStringLiteral("bresca updates check");
        QTest::newRow("in capitals, with extra spaces") << QStringLiteral("  Bresca  UPDATES check ");
        QTest::newRow("being typed") << QStringLiteral("bresca upd");
        QTest::newRow("its first word") << QStringLiteral("bresca");
        QTest::newRow("the start of its first word") << QStringLiteral("bre");
    }

    void testFindsTheCheck()
    {
        QFETCH(QString, query);
        const QList<QueryMatch> matches = launchQuery(query);
        QCOMPARE(matches.size(), 1);
        QCOMPARE(matches.first().text(), QStringLiteral("Check for BrescaOS updates"));
    }

    void testFindsNothing_data()
    {
        QTest::addColumn<QString>("query");
        QTest::newRow("another word") << QStringLiteral("firefox");
        QTest::newRow("one of its words, not first") << QStringLiteral("updates");
        QTest::newRow("its words in another order") << QStringLiteral("bresca check");
        QTest::newRow("more than the command") << QStringLiteral("bresca updates check now");
    }

    void testFindsNothing()
    {
        QFETCH(QString, query);
        QVERIFY(launchQuery(query).isEmpty());
    }
};

QTEST_MAIN(RunnerTest)

#include "runnertest.moc"
