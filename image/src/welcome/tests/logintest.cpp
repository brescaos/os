// The bresca-welcome program, offscreen, with its own state folder: when it
// opens at login (--login), and what it records.
#include "revision.h"

#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>

class LoginTest : public QObject
{
    Q_OBJECT

    QTemporaryDir m_stateFolder;

    QString stateFile() const
    {
        return m_stateFolder.filePath(QStringLiteral("bresca-welcomerc"));
    }

    QByteArray readState() const
    {
        QFile file(stateFile());
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

    void writeState(const QByteArray &content)
    {
        QFile file(stateFile());
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(content);
    }

    static QByteArray stateWithRevision(int revision)
    {
        return "[General]\nShownRevision=" + QByteArray::number(revision) + '\n';
    }

    void start(QProcess &process, const QStringList &arguments)
    {
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert(QStringLiteral("XDG_STATE_HOME"), m_stateFolder.path());
        process.setProcessEnvironment(environment);
        process.setProcessChannelMode(QProcess::ForwardedChannels);
        process.start(QStringLiteral(WELCOME_BINARY), arguments);
        QVERIFY(process.waitForStarted());
    }

    // The window is up: the program is still running after loading its QML.
    void verifyWindowOpens(const QStringList &arguments)
    {
        QProcess process;
        start(process, arguments);
        QVERIFY2(!process.waitForFinished(3000), "bresca-welcome exited instead of showing its window");
        process.kill();
        process.waitForFinished();
    }

    void verifyExitsWithoutWindow(const QStringList &arguments)
    {
        QProcess process;
        start(process, arguments);
        QVERIFY(process.waitForFinished(3000));
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);
    }

private Q_SLOTS:
    void init()
    {
        QVERIFY(m_stateFolder.isValid());
        QFile::remove(stateFile());
    }

    void firstLogin_opensAndRecordsRevision()
    {
        verifyWindowOpens({QStringLiteral("--login")});
        QCOMPARE(readState(), stateWithRevision(Revision::current));
    }

    void olderRevisionShown_opensAndRecordsRevision()
    {
        writeState(stateWithRevision(Revision::current - 1));
        verifyWindowOpens({QStringLiteral("--login")});
        QCOMPARE(readState(), stateWithRevision(Revision::current));
    }

    void currentRevisionShown_exitsWithoutWindow()
    {
        writeState(stateWithRevision(Revision::current));
        verifyExitsWithoutWindow({QStringLiteral("--login")});
        QCOMPARE(readState(), stateWithRevision(Revision::current));
    }

    // After going back to a previous BrescaOS version.
    void newerRevisionShown_exitsWithoutWindow()
    {
        writeState(stateWithRevision(Revision::current + 1));
        verifyExitsWithoutWindow({QStringLiteral("--login")});
        QCOMPARE(readState(), stateWithRevision(Revision::current + 1));
    }

    void fromMenu_opensWithoutRecording()
    {
        writeState(stateWithRevision(Revision::current));
        verifyWindowOpens({});
        QCOMPARE(readState(), stateWithRevision(Revision::current));

        QFile::remove(stateFile());
        verifyWindowOpens({});
        QVERIFY(!QFile::exists(stateFile()));
    }
};

QTEST_GUILESS_MAIN(LoginTest)
#include "logintest.moc"
