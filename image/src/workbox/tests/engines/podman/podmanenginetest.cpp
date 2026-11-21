#include "podmanengine.h"
#include "podmanjob.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QTest>
#include <QTimer>

#include <memory>

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

    void appendEvents(int count)
    {
        QFile file(state.filePath("events"));
        QVERIFY(file.open(QIODevice::Append));
        for (int event = 0; event < count; ++event) {
            file.write("{\"Status\": \"start\"}\n");
        }
    }

    // Runs job to its end: its error, if any.
    template<typename Job>
    QString run(Job *job, QStringList *output = nullptr)
    {
        std::unique_ptr<Job> ownedJob(job);
        ownedJob->setAutoDelete(false);
        if (output) {
            connect(ownedJob.get(), &EngineJob::output, this, [output](const QString &line) {
                output->append(line);
            });
        }
        ownedJob->exec();
        return ownedJob->error() ? ownedJob->errorText() : QString();
    }

private Q_SLOTS:
    void initTestCase()
    {
        qputenv("PATH", FAKES_DIR ":" + qgetenv("PATH"));
        qputenv("FAKE_STATE", QFile::encodeName(state.path()));
    }

    void init()
    {
        for (const QString &name : {"calls", "ps.json", "fail", "exec-status", "delay", "stderr", "events"}) {
            QFile::remove(state.filePath(name));
        }
    }

    void listReadsTheLabels()
    {
        writeState("ps.json",
                   "[{\"Names\": [\"c1\"], \"State\": \"running\", \"Labels\": {\"com.brescaos.workbox.name\": \"Cliente 01\", \"com.brescaos.workbox.color\": \"#3584e4\"}},"
                   " {\"Names\": [\"c2\"], \"State\": \"exited\", \"Labels\": {\"com.brescaos.workbox.name\": \"c2\", \"com.brescaos.workbox.color\": \"#e5a50a\"}}]");
        std::unique_ptr<ListJob> job(PodmanEngine().list());
        job->setAutoDelete(false);
        QVERIFY(job->exec());
        const QList<Workbox> workboxes = job->workboxes();
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
        QCOMPARE(run(PodmanEngine().create({.id = "c1", .name = "Cliente 01", .color = QColor("#3584e4")}, "registry.fedoraproject.org/fedora-toolbox:44")),
                 QString());
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
        QCOMPARE(run(engine.start("c1")), QString());
        QCOMPARE(run(engine.stop("c1")), QString());
        QCOMPARE(calls(), (QList<QStringList>{{"start", "c1"}, {"stop", "--time", "0", "c1"}}));
    }

    // The shell's status is enter's, whatever it is.
    void enter()
    {
        writeState("exec-status", "3");
        std::unique_ptr<EnterJob> job(PodmanEngine().enter("c1", {{"WORKBOX_NAME", "Cliente 01"}}, false));
        job->setAutoDelete(false);
        QVERIFY(job->exec());
        QCOMPARE(job->exitStatus(), 3);
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
        QCOMPARE(run(PodmanEngine().enter("c1", {}, true)), QString());
        QCOMPARE(calls(),
                 (QList<QStringList>{{"exec", "--interactive", "--user", "root", "--workdir", "/root", "--env", "TERM", "c1", "bash", "--login"}}));
    }

    // Its root writes the rule, by the user's uid, and it's left stopped.
    void createWithSudo()
    {
        QCOMPARE(run(PodmanEngine().create({.id = "c1", .name = "c1", .color = Qt::red, .hasSudo = true}, "image")), QString());
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
        QVERIFY(run(PodmanEngine().create({.id = "c1", .name = "c1", .color = Qt::red, .hasSudo = true}, "image")).startsWith("podman exec"));
        const QList<QStringList> podmanCalls = calls();
        QCOMPARE(podmanCalls.mid(podmanCalls.size() - 2),
                 (QList<QStringList>{{"rm", "--force", "--time", "0", "c1"}, {"volume", "rm", "workbox-c1-home"}}));
    }

    // When creating it fails, nothing is cleaned up: what has that name
    // isn't the job's.
    void createFailingCleansNothing()
    {
        writeState("fail", "create\n");
        QCOMPARE(run(PodmanEngine().create({.id = "c1", .name = "c1", .color = Qt::red, .hasSudo = true}, "image")).left(14),
                 QStringLiteral("podman create "));
        QCOMPARE(calls().size(), 1);
    }

    void removeRemovesTheContainerAndItsHome()
    {
        QCOMPARE(run(PodmanEngine().remove("c1")), QString());
        QCOMPARE(calls(), (QList<QStringList>{{"rm", "--force", "--time", "0", "c1"}, {"volume", "rm", "workbox-c1-home"}}));
    }

    void failureIsTheJobsError()
    {
        writeState("fail", "rm\n");
        QCOMPARE(run(PodmanEngine().remove("c1")), QStringLiteral("podman rm --force --time 0 c1 failed"));
        // The home stays when the container couldn't be removed.
        QCOMPARE(calls().size(), 1);
    }

    // Podman's standard error, line by line as it comes, also its last line
    // without a newline.
    void outputIsPodmansStandardError()
    {
        writeState("stderr", "Trying to pull image\nCopying blob\nWriting manifest");
        QStringList output;
        QCOMPARE(run(PodmanEngine().start("c1"), &output), QString());
        QCOMPARE(output, (QStringList{"Trying to pull image", "Copying blob", "Writing manifest"}));
    }

    // The event loop keeps running while podman does: the job never waits.
    void doesntBlock()
    {
        writeState("delay", "0.5");
        int ticks = 0;
        QTimer ticker;
        connect(&ticker, &QTimer::timeout, this, [&ticks] {
            ++ticks;
        });
        ticker.start(20);
        QCOMPARE(run(PodmanEngine().start("c1")), QString());
        QVERIFY2(ticks >= 10, QByteArray::number(ticks));
    }

    // The shell runs while the event loop does.
    void enterDoesntBlock()
    {
        writeState("delay", "0.5");
        int ticks = 0;
        QTimer ticker;
        connect(&ticker, &QTimer::timeout, this, [&ticks] {
            ++ticks;
        });
        ticker.start(20);
        QCOMPARE(run(PodmanEngine().enter("c1", {}, false)), QString());
        QVERIFY2(ticks >= 10, QByteArray::number(ticks));
    }

    // Several events in a row are one change, 100 ms after the last; a later
    // one, another.
    void watch()
    {
        writeState("events", "");
        std::unique_ptr<WatchJob> job(PodmanEngine().watch());
        job->setAutoDelete(false);
        int changes = 0;
        connect(job.get(), &WatchJob::changed, this, [&changes] {
            ++changes;
        });
        job->start();
        QTRY_COMPARE(calls(),
                     (QList<QStringList>{
                         {"events", "--format", "json", "--filter", "type=container", "--filter", "label=com.brescaos.workbox.name"}}));
        // tail starts following the file a moment after it's run.
        QTest::qWait(200);

        appendEvents(3);
        QTRY_COMPARE(changes, 1);
        QTest::qWait(300);
        QCOMPARE(changes, 1);
        appendEvents(1);
        QTRY_COMPARE(changes, 2);
        QVERIFY(job->kill());
    }

    // When podman events ends, it's started again, and that's a change: some
    // may have been missed.
    void watchRestarts()
    {
        writeState("fail", "events\n");
        PodmanWatchJob job({"events"}, std::chrono::milliseconds(100));
        job.setAutoDelete(false);
        int changes = 0;
        connect(&job, &WatchJob::changed, this, [&changes] {
            ++changes;
        });
        job.start();
        QTRY_VERIFY(calls().size() >= 2);
        QTRY_VERIFY(changes >= 1);
        QVERIFY(job.kill());
    }

    // Killed, it stops podman and gives no result.
    void kill()
    {
        writeState("delay", "5");
        auto *job = PodmanEngine().start("c1");
        bool hasResult = false;
        connect(job, &KJob::result, this, [&hasResult] {
            hasResult = true;
        });
        job->start();
        QTRY_COMPARE(calls().size(), 1);
        QElapsedTimer elapsed;
        elapsed.start();
        QVERIFY(job->kill());
        QTest::qWait(100);
        QVERIFY(!hasResult);
        QVERIFY(elapsed.elapsed() < 1000);
    }
};

QTEST_GUILESS_MAIN(PodmanEngineTest)
#include "podmanenginetest.moc"
