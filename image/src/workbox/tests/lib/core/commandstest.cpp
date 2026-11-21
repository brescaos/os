#include "color.h"
#include "commands.h"

#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include <functional>

namespace
{

const QString image = QStringLiteral("registry.fedoraproject.org/fedora-toolbox:44");

// A job that does its work after start() returns, as an engine's would, and
// fails with failure if it's set.
class FakeJob : public ListJob
{
public:
    FakeJob(const std::function<void()> &work, const QList<Workbox> &workboxes = {}, const QString &failure = {})
        : m_work(work)
        , m_failure(failure)
    {
        setWorkboxes(workboxes);
    }

    void start() override
    {
        QMetaObject::invokeMethod(
            this,
            [this] {
                if (!isOutputToTerminal()) {
                    Q_EMIT output(QStringLiteral("fake output"));
                }
                if (!m_failure.isEmpty()) {
                    fail(m_failure);
                    return;
                }
                m_work();
                emitResult();
            },
            Qt::QueuedConnection);
    }

private:
    std::function<void()> m_work;
    QString m_failure;
};

// A shell that exits at once.
class FakeEnterJob : public EnterJob
{
public:
    void start() override
    {
        QMetaObject::invokeMethod(
            this,
            [this] {
                emitResult();
            },
            Qt::QueuedConnection);
    }
};

// Nothing ever changes.
class FakeWatchJob : public WatchJob
{
public:
    void start() override
    {
    }
};

// The workboxes in memory, and the image each one was created from.
class FakeEngine : public Engine
{
public:
    QList<Workbox> workboxes;
    QStringList images;
    QString createFailure;

    ListJob *list() override
    {
        return new FakeJob([] { }, workboxes);
    }

    WatchJob *watch() override
    {
        return new FakeWatchJob;
    }

    EngineJob *create(const Workbox &workbox, const QString &image) override
    {
        return new FakeJob(
            [this, workbox, image] {
                workboxes.append(workbox);
                images.append(image);
            },
            {},
            createFailure);
    }

    EngineJob *start(const QString &) override
    {
        return new FakeJob([] { });
    }

    EngineJob *stop(const QString &) override
    {
        return new FakeJob([] { });
    }

    EngineJob *remove(const Workbox &) override
    {
        return new FakeJob([] { });
    }

    EnterJob *enter(const QString &, const QStringList &, const QMap<QString, QString> &, bool) override
    {
        return new FakeEnterJob;
    }

    QStringList shellCommandLine(const QString &, const QMap<QString, QString> &) override
    {
        return {};
    }
};

// Runs Commands::CreateJob to its end; its workbox, or its error.
std::pair<Workbox, QString> create(FakeEngine &engine,
                                   const QString &id,
                                   const std::optional<QString> &name,
                                   const std::optional<QString> &color,
                                   bool hasSudo = false,
                                   const QStringList &volumes = {})
{
    Commands::CreateJob job(engine, id, {.name = name, .color = color, .hasSudo = hasSudo, .volumes = volumes}, image);
    job.setAutoDelete(false);
    job.exec();
    return {job.workbox(), job.error() ? job.errorText() : QString()};
}

}

class CommandsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void createWithNameAndColor()
    {
        FakeEngine engine;
        const auto [workbox, error] = create(engine, "c1", QStringLiteral("Cliente 01"), QStringLiteral("53,132,228"), true);
        QCOMPARE(error, QString());
        QCOMPARE(workbox.id, QStringLiteral("c1"));
        QCOMPARE(workbox.name, QStringLiteral("Cliente 01"));
        QCOMPARE(workbox.color.name(), QStringLiteral("#3584e4"));
        QVERIFY(engine.workboxes.first().hasSudo);
        QCOMPARE(engine.workboxes.size(), 1);
        QCOMPARE(engine.images, QStringList{image});
    }

    void createDefaultsToItsIdAndThePalette()
    {
        FakeEngine engine;
        const Workbox first = create(engine, "c1", std::nullopt, std::nullopt).first;
        QCOMPARE(first.name, QStringLiteral("c1"));
        QCOMPARE(first.color, WorkboxColor::palette().first());
        QVERIFY(!first.hasSudo);

        const Workbox second = create(engine, "c2", std::nullopt, std::nullopt).first;
        QCOMPARE(second.color, WorkboxColor::pickFromPalette({first.color}));
    }

    void createRefuses_data()
    {
        QTest::addColumn<QString>("id");
        QTest::addColumn<std::optional<QString>>("name");
        QTest::addColumn<std::optional<QString>>("color");
        QTest::addColumn<QString>("message");

        const std::optional<QString> none;
        QTest::newRow("invalid id") << "C1" << none << none << "invalid id \"C1\"";
        QTest::newRow("existing id") << "c0" << none << none << "a workbox with the id \"c0\" already exists";
        QTest::newRow("empty name") << "c1" << std::optional<QString>(" ") << none << "the name can't be empty";
        QTest::newRow("invalid color") << "c1" << none << std::optional<QString>("red") << "invalid color \"red\"";
    }

    void createRefuses()
    {
        QFETCH(QString, id);
        QFETCH(std::optional<QString>, name);
        QFETCH(std::optional<QString>, color);
        QFETCH(QString, message);

        FakeEngine engine;
        engine.workboxes.append({.id = "c0", .name = "c0", .color = Qt::red});
        const QString error = create(engine, id, name, color).second;
        QVERIFY2(error.startsWith(message), qPrintable(error));
        QCOMPARE(engine.workboxes.size(), 1);
    }

    // In the user's home, at their real paths; never twice, nor a blocked
    // one, which creates nothing.
    void createVolumes()
    {
        QTemporaryDir home;
        const QString realHome = QDir(home.path()).canonicalPath();
        QVERIFY(QDir(realHome).mkpath("Clients/01"));
        QVERIFY(QDir(realHome).mkpath(".ssh"));
        const QByteArray previousHome = qgetenv("HOME");
        qputenv("HOME", QFile::encodeName(realHome));

        FakeEngine engine;
        const Workbox workbox = create(engine, "c1", std::nullopt, std::nullopt, false, {realHome + "/Clients/01:" + realHome + "/Clients/01:ro"}).first;
        QCOMPARE(workbox.volumes, (QList<Volume>{{.source = realHome + "/Clients/01", .target = realHome + "/Clients/01", .isReadOnly = true}}));
        QCOMPARE(engine.workboxes.first().volumes, workbox.volumes);

        QVERIFY(create(engine, "c2", std::nullopt, std::nullopt, false, {realHome + "/Clients/01:~/p", "~/p:ro"})
                    .second.startsWith("two volumes are at"));
        // The same folder at two places is fine.
        QCOMPARE(create(engine, "c4", std::nullopt, std::nullopt, false, {realHome + "/Clients/01:~/p", realHome + "/Clients/01:~/project"}).second,
                 QString());
        QVERIFY(create(engine, "c3", std::nullopt, std::nullopt, false, {realHome + "/.ssh:~/keys"}).second.contains("keys"));
        QCOMPARE(engine.workboxes.size(), 2);
        qputenv("HOME", previousHome);
    }

    // Wayland only with a Wayland session.
    void createWayland()
    {
        const QByteArray previousDisplay = qgetenv("WAYLAND_DISPLAY");
        FakeEngine engine;
        qputenv("WAYLAND_DISPLAY", "wayland-0");
        Commands::CreateJob job(engine, "c1", {.hasWayland = true}, image);
        job.setAutoDelete(false);
        QVERIFY(job.exec());
        QVERIFY(engine.workboxes.first().hasWayland);

        qunsetenv("WAYLAND_DISPLAY");
        Commands::CreateJob refused(engine, "c2", {.hasWayland = true}, image);
        refused.setAutoDelete(false);
        QVERIFY(!refused.exec());
        QVERIFY(refused.errorText().contains("no Wayland session"));
        qputenv("WAYLAND_DISPLAY", previousDisplay);
    }

    // The engine's output and error are the job's.
    void createPassesOnTheEngine()
    {
        FakeEngine engine;
        engine.createFailure = QStringLiteral("podman create failed");
        Commands::CreateJob job(engine, "c1", {}, image);
        job.setAutoDelete(false);
        QStringList output;
        connect(&job, &EngineJob::output, this, [&output](const QString &line) {
            output.append(line);
        });
        QVERIFY(!job.exec());
        QCOMPARE(job.errorText(), engine.createFailure);
        QCOMPARE(output, (QStringList{"fake output", "fake output"}));
    }

    // On a terminal, its steps' output goes there too, not to output().
    void createOutputToTerminal()
    {
        FakeEngine engine;
        Commands::CreateJob job(engine, "c1", {}, image);
        job.setAutoDelete(false);
        job.setOutputToTerminal(true);
        QStringList output;
        connect(&job, &EngineJob::output, this, [&output](const QString &line) {
            output.append(line);
        });
        QVERIFY(job.exec());
        QCOMPARE(output, QStringList());
    }

    // Its result comes after start() returns, from the event loop.
    void createDoesntBlock()
    {
        FakeEngine engine;
        auto *job = new Commands::CreateJob(engine, "c1", {}, image);
        bool hasFinished = false;
        connect(job, &KJob::result, this, [&hasFinished] {
            hasFinished = true;
        });
        job->start();
        QVERIFY(!hasFinished);
        QTRY_VERIFY(hasFinished);
    }

    void find()
    {
        const QList<Workbox> workboxes{{.id = "c1", .name = "Cliente 01", .color = Qt::red}};
        QCOMPARE(Commands::find(workboxes, "c1").name, QStringLiteral("Cliente 01"));
        QVERIFY_THROWS_EXCEPTION(WorkboxError, Commands::find(workboxes, "c2"));
    }

    void sortedById()
    {
        QList<Workbox> workboxes;
        for (const QString id : {"c2", "a1", "c10"}) {
            workboxes.append({.id = id, .name = id, .color = Qt::red});
        }
        QStringList ids;
        for (const Workbox &workbox : Commands::sortedById(workboxes)) {
            ids.append(workbox.id);
        }
        QCOMPARE(ids, (QStringList{"a1", "c10", "c2"}));
    }
};

QTEST_GUILESS_MAIN(CommandsTest)
#include "commandstest.moc"
