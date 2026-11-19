// The KRunner plugin's DistroboxJob with the fake distrobox of fakes/
// (FAKES_DIR), on a fresh state folder for each test. Its notifications only
// show in the development VM.
#include "distroboxjob.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

class DistroboxJobTest : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> m_state;

    void writeFile(const QString &file, const QByteArray &content) const
    {
        QFile output(m_state->filePath(file));
        QVERIFY(output.open(QIODevice::WriteOnly | QIODevice::Truncate));
        output.write(content);
    }

    QStringList calls() const
    {
        QFile input(m_state->filePath("calls"));
        if (!input.open(QIODevice::ReadOnly)) {
            return {};
        }
        return QString::fromUtf8(input.readAll()).split('\n', Qt::SkipEmptyParts);
    }

private Q_SLOTS:
    void initTestCase()
    {
        qputenv("PATH", QByteArray(FAKES_DIR) + ':' + qgetenv("PATH"));
    }

    void init()
    {
        m_state = std::make_unique<QTemporaryDir>();
        QVERIFY(m_state->isValid());
        qputenv("FAKE_STATE", m_state->path().toUtf8());
    }

    void describesItselfAndSucceeds()
    {
        const auto job = std::make_unique<DistroboxJob>(&DistroboxCommands::create, "dev", "Creating distrobox “dev”");
        // Read after its result.
        job->setAutoDelete(false);
        QSignalSpy description(job.get(), &KJob::description);
        QSignalSpy result(job.get(), &KJob::result);
        job->start();
        QCOMPARE(description.first().at(1).toString(), QString("Creating distrobox “dev”"));
        QVERIFY(result.wait());
        QCOMPARE(job->error(), int(KJob::NoError));
        QCOMPARE(calls(), QStringList{"distrobox create --yes --name dev"});
    }

    void failsWithWhatDistroboxSaid()
    {
        writeFile("rm.fails", "no such container");
        const auto job = std::make_unique<DistroboxJob>(&DistroboxCommands::remove, "dev", "Removing distrobox “dev”");
        // Read after its result.
        job->setAutoDelete(false);
        QSignalSpy result(job.get(), &KJob::result);
        job->start();
        QVERIFY(result.wait());
        QCOMPARE(job->error(), int(KJob::UserDefinedError));
        QCOMPARE(job->errorText(), QString("no such container"));
        QCOMPARE(calls(), QStringList{"distrobox rm --force dev"});
    }
};

QTEST_GUILESS_MAIN(DistroboxJobTest)

#include "distroboxjobtest.moc"
