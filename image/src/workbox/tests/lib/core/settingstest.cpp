#include "settings.h"
#include "workbox.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class SettingsTest : public QObject
{
    Q_OBJECT

    QTemporaryDir directory;
    // workbox's own, a distribution's, the user's.
    QStringList files;

    void write(int level, const QByteArray &content)
    {
        QFile file(files.at(level));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(content);
    }

private Q_SLOTS:
    void init()
    {
        files = {directory.filePath("workbox.conf"), directory.filePath("distribution.conf"), directory.filePath("user.conf")};
        for (const QString &file : std::as_const(files)) {
            QFile::remove(file);
        }
        write(0, "[General]\nengine=podman\nimage=registry.fedoraproject.org/fedora-toolbox:latest\n");
    }

    void workboxsOwnAlone()
    {
        const Settings settings = readSettings(files);
        QCOMPARE(settings.engine, QStringLiteral("podman"));
        QCOMPARE(settings.image, QStringLiteral("registry.fedoraproject.org/fedora-toolbox:latest"));
        QVERIFY(!QFile::exists(files.at(2)));
    }

    // The distribution's over workbox's, the user's over both, key by key.
    void eachLevelWinsKeyByKey()
    {
        write(1, "[General]\nimage=registry.fedoraproject.org/fedora-toolbox:44\n");
        QCOMPARE(readSettings(files).image, QStringLiteral("registry.fedoraproject.org/fedora-toolbox:44"));

        write(2, "[General]\nimage=quay.io/fedora/fedora:44\n");
        const Settings settings = readSettings(files);
        QCOMPARE(settings.engine, QStringLiteral("podman"));
        QCOMPARE(settings.image, QStringLiteral("quay.io/fedora/fedora:44"));
    }

    void missingKeyFails()
    {
        write(0, "[General]\nengine=podman\n");
        QVERIFY_THROWS_EXCEPTION(WorkboxError, readSettings(files));
    }

    // Where they are: the user's in their config folder.
    void levels()
    {
        const QStringList levels = settingsFiles();
        QCOMPARE(levels.size(), 3);
        QVERIFY(levels.at(0).endsWith("/workbox/workbox.conf"));
        QVERIFY(levels.at(1).endsWith("/defaults/workbox/workbox.conf"));
        QVERIFY(levels.at(2).endsWith("/workbox/workbox.conf"));
    }
};

QTEST_GUILESS_MAIN(SettingsTest)
#include "settingstest.moc"
