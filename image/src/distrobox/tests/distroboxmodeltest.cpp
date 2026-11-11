#include "distroboxmodel.h"

#include "terminalcolors.h"

#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPalette>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using Operation = DistroboxModel::Operation;

// DistroboxModel with the fake podman and distrobox of fakes/ (FAKES_DIR), on
// a fresh state folder for each test.
class DistroboxModelTest : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> m_state;

    QString statePath(const QString &file) const
    {
        return m_state->filePath(file);
    }

    void writeFile(const QString &file, const QByteArray &content) const
    {
        QFile output(statePath(file));
        QVERIFY(output.open(QIODevice::WriteOnly | QIODevice::Truncate));
        output.write(content);
    }

    void appendToFile(const QString &file, const QByteArray &content) const
    {
        QFile output(statePath(file));
        QVERIFY(output.open(QIODevice::WriteOnly | QIODevice::Append));
        output.write(content);
    }

    // podman ps's distroboxes: {name, state}, in podman's order.
    void writeListing(const QList<std::pair<QString, QString>> &distroboxes) const
    {
        QJsonArray containers;
        for (const auto &[name, state] : distroboxes) {
            containers.append(QJsonObject{{"Names", QJsonArray{name}}, {"State", state}});
        }
        writeFile("ps.json", QJsonDocument(containers).toJson());
    }

    QStringList calls() const
    {
        QFile input(statePath("calls"));
        if (!input.open(QIODevice::ReadOnly)) {
            return {};
        }
        return QString::fromUtf8(input.readAll()).split('\n', Qt::SkipEmptyParts);
    }

    QStringList distroboxCalls() const
    {
        return calls().filter(QRegularExpression("^distrobox "));
    }

    static QVariant valueOf(const DistroboxModel &model, int row, DistroboxModel::Role role)
    {
        return model.data(model.index(row), role);
    }

    static QStringList names(const DistroboxModel &model)
    {
        QStringList names;
        for (int row = 0; row < model.rowCount(); ++row) {
            names << valueOf(model, row, DistroboxModel::NameRole).toString();
        }
        return names;
    }

    static int operationOf(const DistroboxModel &model, int row)
    {
        return valueOf(model, row, DistroboxModel::OperationRole).toInt();
    }

private Q_SLOTS:
    void init()
    {
        m_state = std::make_unique<QTemporaryDir>();
        QVERIFY(m_state->isValid());
        qputenv("FAKE_STATE", m_state->path().toUtf8());
        writeFile("events", {});
        writeFile("inspect", "true\n");
        writeListing({{"web", "exited"}, {"dev", "running"}, {"build", "created"}});
    }

    void initTestCase()
    {
        qputenv("PATH", QByteArray(FAKES_DIR) + ':' + qgetenv("PATH"));
    }

    void listsEveryDistroboxByName()
    {
        DistroboxModel model;
        QTRY_COMPARE(model.rowCount(), 3);
        QCOMPARE(names(model), (QStringList{"build", "dev", "web"}));
        QCOMPARE(valueOf(model, 0, DistroboxModel::RunningRole).toBool(), false);
        QCOMPARE(valueOf(model, 1, DistroboxModel::RunningRole).toBool(), true);
        QCOMPARE(model.runningCount(), 1);
        QCOMPARE(operationOf(model, 0), static_cast<int>(Operation::None));
    }

    void listsAgainAfterAnEvent()
    {
        DistroboxModel model;
        QTRY_COMPARE(model.rowCount(), 3);
        QSignalSpy runningCountChanged(&model, &DistroboxModel::runningCountChanged);
        writeListing({{"web", "running"}, {"dev", "running"}, {"new", "created"}});
        appendToFile("events", "container start\n");
        QTRY_COMPARE(names(model), (QStringList{"dev", "new", "web"}));
        QCOMPARE(model.runningCount(), 2);
        QCOMPARE(runningCountChanged.count(), 1);
    }

    // Konsole's color for its tab, with the palette's highlight as accent.
    void colorIsKonsoles()
    {
        DistroboxModel model;
        QTRY_COMPARE(model.rowCount(), 3);
        const QColor accent = qGuiApp->palette().color(QPalette::Active, QPalette::Highlight);
        QCOMPARE(valueOf(model, 1, DistroboxModel::ColorRole).value<QColor>(), TerminalColors::konsoleTabColor("dev", accent));
    }

    void startsWithItsSpinnerMeanwhile()
    {
        DistroboxModel model;
        QTRY_COMPARE(model.rowCount(), 3);
        model.start("web");
        QCOMPARE(operationOf(model, 2), static_cast<int>(Operation::Starting));
        QTRY_COMPARE(operationOf(model, 2), static_cast<int>(Operation::None));
        QCOMPARE(valueOf(model, 2, DistroboxModel::RunningRole).toBool(), true);
        QCOMPARE(model.runningCount(), 2);
        QCOMPARE(distroboxCalls(), QStringList{"distrobox enter web -- true"});
    }

    void ignoresAnotherOperationMeanwhile()
    {
        DistroboxModel model;
        QTRY_COMPARE(model.rowCount(), 3);
        model.start("web");
        model.remove("web");
        QTRY_COMPARE(operationOf(model, 2), static_cast<int>(Operation::None));
        QCOMPARE(distroboxCalls(), QStringList{"distrobox enter web -- true"});
    }

    void stops()
    {
        DistroboxModel model;
        QTRY_COMPARE(model.rowCount(), 3);
        model.stop("dev");
        QCOMPARE(operationOf(model, 1), static_cast<int>(Operation::Stopping));
        QTRY_COMPARE(operationOf(model, 1), static_cast<int>(Operation::None));
        QCOMPARE(valueOf(model, 1, DistroboxModel::RunningRole).toBool(), false);
        QCOMPARE(model.runningCount(), 0);
        QCOMPARE(distroboxCalls(), QStringList{"distrobox stop --yes dev"});
    }

    // With a terminal open in it, podman stops it but fails.
    void stoppedAfterAFailureIsStopped()
    {
        writeFile("stop.fails", "has active exec sessions, refusing to clean up");
        writeFile("inspect", "false\n");
        DistroboxModel model;
        QTRY_COMPARE(model.rowCount(), 3);
        model.stop("dev");
        QTRY_COMPARE(operationOf(model, 1), static_cast<int>(Operation::None));
        QCOMPARE(valueOf(model, 1, DistroboxModel::RunningRole).toBool(), false);
        QCOMPARE(valueOf(model, 1, DistroboxModel::FailedOperationRole).toInt(), static_cast<int>(Operation::None));
        QVERIFY(calls().contains("podman inspect --format {{.State.Running}} dev"));
    }

    void aFailureStaysUntilTheNextOperation()
    {
        writeFile("stop.fails", "Error: something went wrong");
        DistroboxModel model;
        QTRY_COMPARE(model.rowCount(), 3);
        model.stop("dev");
        QTRY_COMPARE(operationOf(model, 1), static_cast<int>(Operation::None));
        QCOMPARE(valueOf(model, 1, DistroboxModel::RunningRole).toBool(), true);
        QCOMPARE(valueOf(model, 1, DistroboxModel::FailedOperationRole).toInt(), static_cast<int>(Operation::Stopping));
        QCOMPARE(valueOf(model, 1, DistroboxModel::ErrorDetailsRole).toString(), QString("Error: something went wrong"));

        QFile::remove(statePath("stop.fails"));
        model.stop("dev");
        QCOMPARE(valueOf(model, 1, DistroboxModel::FailedOperationRole).toInt(), static_cast<int>(Operation::None));
        QCOMPARE(valueOf(model, 1, DistroboxModel::ErrorDetailsRole).toString(), QString());
        QTRY_COMPARE(operationOf(model, 1), static_cast<int>(Operation::None));
    }

    void removesItsRowOnceRemoved()
    {
        DistroboxModel model;
        QTRY_COMPARE(model.rowCount(), 3);
        model.remove("dev");
        QCOMPARE(operationOf(model, 1), static_cast<int>(Operation::Removing));
        QTRY_COMPARE(names(model), (QStringList{"build", "web"}));
        QCOMPARE(model.runningCount(), 0);
        QCOMPARE(distroboxCalls(), QStringList{"distrobox rm --force dev"});
    }

    void aFailedRemovalKeepsItsRow()
    {
        writeFile("rm.fails", "Error: no");
        DistroboxModel model;
        QTRY_COMPARE(model.rowCount(), 3);
        model.remove("dev");
        QTRY_COMPARE(operationOf(model, 1), static_cast<int>(Operation::None));
        QCOMPARE(names(model), (QStringList{"build", "dev", "web"}));
        QCOMPARE(valueOf(model, 1, DistroboxModel::FailedOperationRole).toInt(), static_cast<int>(Operation::Removing));
    }

    void ignoresAnUnknownName()
    {
        DistroboxModel model;
        QTRY_COMPARE(model.rowCount(), 3);
        model.start("missing");
        QTest::qWait(100);
        QCOMPARE(distroboxCalls(), QStringList());
    }
};

QTEST_MAIN(DistroboxModelTest)
#include "distroboxmodeltest.moc"
