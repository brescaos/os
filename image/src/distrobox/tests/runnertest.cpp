// What the KRunner plugin offers for each query, with the fake podman of
// fakes/ (FAKES_DIR) listing web (stopped) and dev and devtools (running).
// What choosing it does (the dialog, the notifications) only runs in the
// development VM; its commands have their tests in distroboxjobtest.cpp.
#include <KRunner/AbstractRunnerTest>
#include <KRunner/QueryMatch>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>

using namespace KRunner;

class RunnerTest : public AbstractRunnerTest
{
    Q_OBJECT

private:
    QTemporaryDir m_state;

    // Its results' texts, sorted, with whether each can be chosen: "-" in
    // front when it can't.
    QStringList textsFor(const QString &query)
    {
        QStringList texts;
        for (const QueryMatch &match : launchQuery(query)) {
            texts << (match.isEnabled() ? QString() : QStringLiteral("-")) + match.text();
        }
        texts.sort();
        return texts;
    }

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_state.isValid());
        qputenv("FAKE_STATE", m_state.path().toUtf8());
        qputenv("PATH", QByteArray(FAKES_DIR) + ':' + qgetenv("PATH"));
        QJsonArray containers;
        for (const auto &[name, state] : {std::pair{"web", "exited"}, {"dev", "running"}, {"devtools", "running"}}) {
            containers.append(QJsonObject{{"Names", QJsonArray{name}}, {"State", state}});
        }
        QFile listing(m_state.filePath("ps.json"));
        QVERIFY(listing.open(QIODevice::WriteOnly));
        listing.write(QJsonDocument(containers).toJson());
        listing.close();
        initProperties();
    }

    void findsTheCommands_data()
    {
        QTest::addColumn<QString>("query");
        QTest::addColumn<QStringList>("texts");
        const QString createNew = QStringLiteral("Create distrobox “new”");
        QTest::newRow("create") << QStringLiteral("distrobox create new") << QStringList{createNew};
        QTest::newRow("create, initials") << QStringLiteral("dc new") << QStringList{createNew};
        QTest::newRow("create, in capitals, with extra spaces") << QStringLiteral("  Distrobox  CREATE new ") << QStringList{createNew};
        QTest::newRow("create, the name keeps its case")
            << QStringLiteral("dc New") << QStringList{QStringLiteral("Create distrobox “New”")};
        QTest::newRow("create, a name that exists") << QStringLiteral("dc dev") << QStringList{QStringLiteral("-Distrobox “dev” already exists")};
        QTest::newRow("create, a name podman refuses")
            << QStringLiteral("dc -dev") << QStringList{QStringLiteral("-“-dev” can't be a distrobox's name")};
        QTest::newRow("stop, the running ones it's the start of")
            << QStringLiteral("distrobox stop d") << QStringList{QStringLiteral("Stop distrobox “dev”"), QStringLiteral("Stop distrobox “devtools”")};
        QTest::newRow("stop, initials") << QStringLiteral("ds devt") << QStringList{QStringLiteral("Stop distrobox “devtools”")};
        QTest::newRow("stop, not a stopped one") << QStringLiteral("ds web") << QStringList{};
        QTest::newRow("rm, running or not") << QStringLiteral("distrobox rm w") << QStringList{QStringLiteral("Remove distrobox “web”")};
        QTest::newRow("rm, initials")
            << QStringLiteral("dr dev") << QStringList{QStringLiteral("Remove distrobox “dev”"), QStringLiteral("Remove distrobox “devtools”")};
    }

    void findsTheCommands()
    {
        QFETCH(QString, query);
        QFETCH(QStringList, texts);
        texts.sort();
        QCOMPARE(textsFor(query), texts);
    }

    void wholeNameFirst()
    {
        QList<QueryMatch> matches = launchQuery(QStringLiteral("ds dev"));
        std::ranges::sort(matches, [](const QueryMatch &a, const QueryMatch &b) {
            return a.relevance() > b.relevance();
        });
        QCOMPARE(matches.size(), 2);
        QCOMPARE(matches.first().text(), QStringLiteral("Stop distrobox “dev”"));
    }

    void findsNothing_data()
    {
        QTest::addColumn<QString>("query");
        QTest::newRow("another word") << QStringLiteral("firefox");
        QTest::newRow("distrobox alone") << QStringLiteral("distrobox");
        QTest::newRow("a command without a name") << QStringLiteral("distrobox stop");
        QTest::newRow("initials without a name") << QStringLiteral("ds");
        QTest::newRow("another command") << QStringLiteral("distrobox enter dev");
        QTest::newRow("more than a name") << QStringLiteral("dc new box");
        QTest::newRow("initials joined to the name") << QStringLiteral("dsdev");
    }

    void findsNothing()
    {
        QFETCH(QString, query);
        QVERIFY(launchQuery(query).isEmpty());
    }
};

QTEST_MAIN(RunnerTest)

#include "runnertest.moc"
