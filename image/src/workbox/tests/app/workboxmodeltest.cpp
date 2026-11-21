#include "workboxmodel.h"

#include <QAbstractItemModelTester>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

// The app's list on the fake podman of fakes/, with the user's settings
// naming it.
class WorkboxModelTest : public QObject
{
    Q_OBJECT

    QTemporaryDir state;

    void writeState(const QString &name, const QByteArray &content)
    {
        QFile file(state.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(content);
    }

    void appendEvent()
    {
        QFile file(state.filePath("events"));
        QVERIFY(file.open(QIODevice::Append));
        file.write("{\"Status\": \"create\"}\n");
    }

    static QByteArray container(const char *id, const char *name, const char *color, bool isRunning)
    {
        return QStringLiteral("{\"Names\": [\"%1\"], \"State\": \"%2\", \"Labels\": {\"com.brescaos.workbox.name\": \"%3\", "
                              "\"com.brescaos.workbox.color\": \"%4\"}}")
            .arg(QLatin1String(id), QLatin1String(isRunning ? "running" : "exited"), QLatin1String(name), QLatin1String(color))
            .toUtf8();
    }

    int podmanCalls(const QString &command)
    {
        QFile file(state.filePath("calls"));
        if (!file.open(QIODevice::ReadOnly)) {
            return 0;
        }
        return static_cast<int>(QString::fromUtf8(file.readAll()).split("\n\n").count(command + "\n" + "c1"));
    }

    QStringList ids(const WorkboxModel &model)
    {
        QStringList ids;
        for (int row = 0; row < model.rowCount(); ++row) {
            ids.append(model.index(row).data(WorkboxModel::IdRole).toString());
        }
        return ids;
    }

    // Its row's role.
    QVariant data(const WorkboxModel &model, const QString &id, WorkboxModel::Role role)
    {
        const int row = static_cast<int>(ids(model).indexOf(id));
        return row < 0 ? QVariant() : model.index(row).data(role);
    }

private Q_SLOTS:
    void initTestCase()
    {
        qputenv("PATH", FAKES_DIR ":" + qgetenv("PATH"));
        qputenv("FAKE_STATE", QFile::encodeName(state.path()));
        qputenv("XDG_CONFIG_HOME", QFile::encodeName(state.filePath("config")));
        QVERIFY(QDir(state.path()).mkpath("config/workbox"));
    }

    void init()
    {
        for (const QString &name : {"calls", "ps.json", "fail", "delay", "events"}) {
            QFile::remove(state.filePath(name));
        }
        writeState("config/workbox/workbox.conf", "[General]\nengine=podman\nimage=image\n");
        writeState("events", "");
        writeState("ps.json",
                   "[" + container("c2", "Cliente 02", "#e5a50a", true) + ", " + container("c1", "Cliente 01", "#3584e4", false) + "]");
    }

    // By id, with each one's name, color and state; again on an event.
    void listsAndFollows()
    {
        WorkboxModel model;
        QAbstractItemModelTester tester(&model);
        QTRY_COMPARE(ids(model), (QStringList{"c1", "c2"}));
        QCOMPARE(data(model, "c1", WorkboxModel::NameRole).toString(), QStringLiteral("Cliente 01"));
        QCOMPARE(data(model, "c1", WorkboxModel::ColorRole).value<QColor>(), QColor("#3584e4"));
        QCOMPARE(data(model, "c1", WorkboxModel::WorkboxRole).value<Workbox>().name, QStringLiteral("Cliente 01"));
        QVERIFY(!data(model, "c1", WorkboxModel::RunningRole).toBool());
        QCOMPARE(model.runningCount(), 1);
        QCOMPARE(model.problem(), QString());

        // tail follows the file a moment after it starts.
        QTest::qWait(200);
        writeState("ps.json", "[" + container("c3", "c3", "#2ec27e", true) + "]");
        appendEvent();
        QTRY_COMPARE(ids(model), QStringList{"c3"});
        QCOMPARE(model.runningCount(), 1);
    }

    // Its new state at once, and one operation at a time per workbox.
    void start()
    {
        writeState("delay", "0.3");
        WorkboxModel model;
        QTRY_COMPARE(model.rowCount(), 2);
        model.start("c1");
        QCOMPARE(data(model, "c1", WorkboxModel::OperationRole).toInt(), static_cast<int>(WorkboxModel::Operation::Starting));
        model.start("c1");
        QTRY_COMPARE(data(model, "c1", WorkboxModel::OperationRole).toInt(), static_cast<int>(WorkboxModel::Operation::None));
        QVERIFY(data(model, "c1", WorkboxModel::RunningRole).toBool());
        QCOMPARE(model.runningCount(), 2);
        QCOMPARE(podmanCalls("start"), 1);
    }

    // The failure stays in the row, with the job's error.
    void failure()
    {
        WorkboxModel model;
        QTRY_COMPARE(model.rowCount(), 2);
        writeState("fail", "stop\n");
        model.stop("c2");
        QTRY_COMPARE(data(model, "c2", WorkboxModel::FailedOperationRole).toInt(), static_cast<int>(WorkboxModel::Operation::Stopping));
        QCOMPARE(data(model, "c2", WorkboxModel::ErrorTextRole).toString(), QStringLiteral("podman stop --time 0 c2 failed"));
        QVERIFY(data(model, "c2", WorkboxModel::RunningRole).toBool());
    }

    void remove()
    {
        WorkboxModel model;
        QTRY_COMPARE(model.rowCount(), 2);
        model.remove("c1");
        QTRY_COMPARE(ids(model), QStringList{"c2"});
    }

    // podman exec's login shell, with the workbox's prompt; none for a
    // workbox that isn't there.
    void shellCommandLine()
    {
        WorkboxModel model;
        QTRY_COMPARE(model.rowCount(), 2);
        const QStringList commandLine = model.shellCommandLine("c1");
        QCOMPARE(commandLine.mid(0, 2), (QStringList{"podman", "exec"}));
        QVERIFY(commandLine.contains("WORKBOX_NAME=Cliente 01"));
        QCOMPARE(commandLine.mid(commandLine.size() - 3), (QStringList{"c1", "bash", "--login"}));
        QCOMPARE(model.shellCommandLine("c9"), QStringList());
    }

    // The event loop keeps running while podman does.
    void doesntBlock()
    {
        writeState("delay", "0.5");
        int ticks = 0;
        QTimer ticker;
        connect(&ticker, &QTimer::timeout, this, [&ticks] {
            ++ticks;
        });
        ticker.start(20);
        WorkboxModel model;
        QCOMPARE(model.rowCount(), 0);
        QTRY_COMPARE(model.rowCount(), 2);
        QVERIFY2(ticks >= 10, QByteArray::number(ticks));
    }

    // Without settings, no list, and why.
    void problem()
    {
        QFile::remove(state.filePath("config/workbox/workbox.conf"));
        WorkboxModel model;
        QCOMPARE(model.rowCount(), 0);
        QVERIFY(model.problem().startsWith("no engine in "));
    }
};

QTEST_GUILESS_MAIN(WorkboxModelTest)
#include "workboxmodeltest.moc"
