#include "podmanengine.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <unistd.h>

class PodmanEngineTest : public QObject
{
    Q_OBJECT

    QTemporaryDir state;

    void writeState(const QString &name, const QByteArray &content)
    {
        QFile file(state.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(content);
    }

    // Each call's arguments, in order.
    QList<QStringList> calls()
    {
        QFile file(state.filePath("calls"));
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        QList<QStringList> calls;
        for (const QString &call : QString::fromUtf8(file.readAll()).split("\n\n", Qt::SkipEmptyParts)) {
            calls.append(call.split('\n'));
        }
        return calls;
    }

private Q_SLOTS:
    void initTestCase()
    {
        qputenv("PATH", FAKES_DIR ":" + qgetenv("PATH"));
        qputenv("FAKE_STATE", QFile::encodeName(state.path()));
    }

    void init()
    {
        for (const QString &name : {"calls", "ps.json", "fail", "exec-status"}) {
            QFile::remove(state.filePath(name));
        }
    }

    void listReadsTheLabels()
    {
        writeState("ps.json",
                   "[{\"Names\": [\"c1\"], \"State\": \"running\", \"Labels\": {\"com.brescaos.workbox.name\": \"Cliente 01\", \"com.brescaos.workbox.color\": \"#3584e4\"}},"
                   " {\"Names\": [\"c2\"], \"State\": \"exited\", \"Labels\": {\"com.brescaos.workbox.name\": \"c2\", \"com.brescaos.workbox.color\": \"#e5a50a\"}}]");
        const QList<Workbox> workboxes = PodmanEngine().list();
        QCOMPARE(workboxes.size(), 2);
        QCOMPARE(workboxes[0].id, QStringLiteral("c1"));
        QCOMPARE(workboxes[0].name, QStringLiteral("Cliente 01"));
        QCOMPARE(workboxes[0].color, QColor("#3584e4"));
        QVERIFY(workboxes[0].isRunning);
        QCOMPARE(workboxes[1].id, QStringLiteral("c2"));
        QVERIFY(!workboxes[1].isRunning);
        QCOMPARE(calls(), (QList<QStringList>{{"ps", "--all", "--filter", "label=com.brescaos.workbox.name", "--format", "json"}}));
    }

    void create()
    {
        PodmanEngine().create({.id = "c1", .name = "Cliente 01", .color = QColor("#3584e4")}, "registry.fedoraproject.org/fedora-toolbox:44");
        QCOMPARE(calls(),
                 (QList<QStringList>{{"create",
                                      "--name",
                                      "c1",
                                      "--hostname",
                                      "c1",
                                      "--userns=keep-id",
                                      "--network=pasta",
                                      "--volume",
                                      "workbox-c1-home:" + QDir::homePath(),
                                      "--label",
                                      "com.brescaos.workbox.color=#3584e4",
                                      "--label",
                                      "com.brescaos.workbox.name=Cliente 01",
                                      "registry.fedoraproject.org/fedora-toolbox:44",
                                      "sleep",
                                      "infinity"}}));
    }

    void startAndStop()
    {
        PodmanEngine engine;
        engine.start("c1");
        engine.stop("c1");
        QCOMPARE(calls(), (QList<QStringList>{{"start", "c1"}, {"stop", "--time", "0", "c1"}}));
    }

    // The shell's status is enter's, whatever it is.
    void enter()
    {
        writeState("exec-status", "3");
        QCOMPARE(PodmanEngine().enter("c1", {{"WORKBOX_NAME", "Cliente 01"}}, false), 3);
        QCOMPARE(calls(),
                 (QList<QStringList>{{"exec",
                                      "--interactive",
                                      "--workdir",
                                      QDir::homePath(),
                                      "--env",
                                      "TERM",
                                      "--env",
                                      "WORKBOX_NAME=Cliente 01",
                                      "c1",
                                      "bash",
                                      "--login"}}));
    }

    void enterAsRoot()
    {
        PodmanEngine().enter("c1", {}, true);
        QCOMPARE(calls(),
                 (QList<QStringList>{{"exec", "--interactive", "--user", "root", "--workdir", "/root", "--env", "TERM", "c1", "bash", "--login"}}));
    }

    // Its root writes the rule, by the user's uid, and it's left stopped.
    void createWithSudo()
    {
        PodmanEngine().create({.id = "c1", .name = "c1", .color = Qt::red, .hasSudo = true}, "image");
        const QList<QStringList> podmanCalls = calls();
        QCOMPARE(podmanCalls.size(), 4);
        QCOMPARE(podmanCalls[0].first(), QStringLiteral("create"));
        QVERIFY(podmanCalls[0].contains("com.brescaos.workbox.sudo=true"));
        QCOMPARE(podmanCalls[1], (QStringList{"start", "c1"}));
        QCOMPARE(podmanCalls[2].mid(0, 4), (QStringList{"exec", "--user", "root", "c1"}));
        QCOMPARE(podmanCalls[2].last(), QStringLiteral("#%1 ALL=(ALL) NOPASSWD: ALL").arg(getuid()));
        QCOMPARE(podmanCalls[3], (QStringList{"stop", "--time", "0", "c1"}));
    }

    // Nothing of it is left when sudo's rule can't be written.
    void createWithSudoFailing()
    {
        writeState("fail", "exec\n");
        QVERIFY_THROWS_EXCEPTION(WorkboxError, PodmanEngine().create({.id = "c1", .name = "c1", .color = Qt::red, .hasSudo = true}, "image"));
        const QList<QStringList> podmanCalls = calls();
        QCOMPARE(podmanCalls.mid(podmanCalls.size() - 2),
                 (QList<QStringList>{{"rm", "--force", "--time", "0", "c1"}, {"volume", "rm", "workbox-c1-home"}}));
    }

    void removeRemovesTheContainerAndItsHome()
    {
        PodmanEngine().remove("c1");
        QCOMPARE(calls(), (QList<QStringList>{{"rm", "--force", "--time", "0", "c1"}, {"volume", "rm", "workbox-c1-home"}}));
    }

    void failureThrows()
    {
        writeState("fail", "rm\n");
        QVERIFY_THROWS_EXCEPTION(WorkboxError, PodmanEngine().remove("c1"));
        // The home stays when the container couldn't be removed.
        QCOMPARE(calls().size(), 1);
    }
};

QTEST_GUILESS_MAIN(PodmanEngineTest)
#include "podmanenginetest.moc"
