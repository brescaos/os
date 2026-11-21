#include "settings.h"
#include "workbox.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class SettingsTest : public QObject
{
    Q_OBJECT

    QTemporaryDir directory;
    QString systemFile;
    QString userFile;

    void write(const QString &path, const QByteArray &content)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(content);
    }

private Q_SLOTS:
    void init()
    {
        systemFile = directory.filePath("system.conf");
        userFile = directory.filePath("user.conf");
        QFile::remove(systemFile);
        QFile::remove(userFile);
        write(systemFile, "[General]\nengine=podman\nimage=registry.fedoraproject.org/fedora-toolbox:44\n");
    }

    void withoutUserFileTheSystemOnes()
    {
        const Settings settings = readSettings(systemFile, userFile);
        QCOMPARE(settings.engine, QStringLiteral("podman"));
        QCOMPARE(settings.image, QStringLiteral("registry.fedoraproject.org/fedora-toolbox:44"));
        QVERIFY(!QFile::exists(userFile));
    }

    void userFileWinsKeyByKey()
    {
        write(userFile, "[General]\nimage=quay.io/fedora/fedora:44\n");
        const Settings settings = readSettings(systemFile, userFile);
        QCOMPARE(settings.engine, QStringLiteral("podman"));
        QCOMPARE(settings.image, QStringLiteral("quay.io/fedora/fedora:44"));
    }

    void missingKeyFails()
    {
        write(systemFile, "[General]\nengine=podman\n");
        QVERIFY_THROWS_EXCEPTION(WorkboxError, readSettings(systemFile, userFile));
    }
};

QTEST_GUILESS_MAIN(SettingsTest)
#include "settingstest.moc"
