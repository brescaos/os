#include "volumes.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

// parseVolume in a home of its own, with the folders it blocks.
class VolumesTest : public QObject
{
    Q_OBJECT

    QTemporaryDir directory;
    QString home;

private Q_SLOTS:
    void initTestCase()
    {
        home = QDir(directory.path()).canonicalPath() + "/home";
        for (const char *folder : {"Clients/01", ".ssh", ".config/gh", ".config/autostart", ".local/share/applications", ".local/share/fonts", "with:colon"}) {
            QVERIFY(QDir(home).mkpath(folder));
        }
        QVERIFY(QDir(directory.path()).mkpath("outside"));
        QFile notes(home + "/notes.txt");
        QVERIFY(notes.open(QIODevice::WriteOnly));
        QVERIFY(QFile::link(home + "/.ssh", home + "/keys"));
        QVERIFY(QFile::link(home + "/Clients/01", home + "/client"));
        QVERIFY(QFile::link(home + "/with:colon", home + "/colon"));
        QDir::setCurrent(home);
    }

    void volumes_data()
    {
        QTest::addColumn<QString>("argument");
        QTest::addColumn<QString>("source");
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("target");
        QTest::addColumn<bool>("isReadOnly");

        const QString client = home + "/Clients/01";
        const QString project = home + "/project";
        QTest::newRow("its own, as Podman's -v target") << "~/data" << "" << "" << home + "/data" << false;
        QTest::newRow("its own, read-only") << home + "/data:ro" << "" << "" << home + "/data" << true;
        QTest::newRow("absolute") << client + ":" + client << client << "" << client << false;
        QTest::newRow("relative") << "Clients/01:~/project" << client << "" << project << false;
        QTest::newRow("~ in the folder") << "~/Clients/01:~/project" << client << "" << project << false;
        QTest::newRow("through a link, at its real path") << "./client:~/project" << client << "" << project << false;
        QTest::newRow("read-only") << "Clients/01:~/project:ro" << client << "" << project << true;
        QTest::newRow("rw and z") << "Clients/01:~/project:rw,z" << client << "" << project << false;
        QTest::newRow("a target's last /") << "Clients/01:~/project/" << client << "" << project << false;
        QTest::newRow("an app's settings, read-only") << ".config/gh:~/.config/gh:ro" << home + "/.config/gh" << "" << home + "/.config/gh" << true;
        QTest::newRow("a volume of its own, named") << "cache:~/cache" << "" << "cache" << home + "/cache" << false;
        QTest::newRow("a named one, read-only") << "cache:~/cache:ro" << "" << "cache" << home + "/cache" << true;
        QTest::newRow("in .local, read-only") << ".local/share/fonts:~/fonts:ro" << home + "/.local/share/fonts" << "" << home + "/fonts" << true;
    }

    void volumes()
    {
        QFETCH(QString, argument);
        QFETCH(QString, source);
        QFETCH(QString, name);
        QFETCH(QString, target);
        QFETCH(bool, isReadOnly);
        QCOMPARE(parseVolume(argument, home), (Volume{.source = source, .name = name, .target = target, .isReadOnly = isReadOnly}));
    }

    void refuses_data()
    {
        QTest::addColumn<QString>("argument");
        QTest::addColumn<QString>("why");

        QTest::newRow("missing") << "Missing/x:~/x" << "it isn't a folder";
        QTest::newRow("a file") << "./notes.txt:~/x" << "it isn't a folder";
        QTest::newRow("outside home") << directory.path() + "/outside:~/x" << "it's outside the home folder";
        QTest::newRow("home") << home + ":~/x" << "it's the whole home folder";
        QTest::newRow("keys") << "./.ssh:~/keys:ro" << "it holds keys";
        QTest::newRow("keys, through a link") << "./keys:~/keys:ro" << "it holds keys";
        QTest::newRow("what runs at login") << ".config/autostart:~/a:ro" << "it holds keys";
        QTest::newRow("holding what runs at login") << ".local/share:~/s" << "it holds " + home + "/.local/share/applications";
        QTest::newRow(".config whole") << "./.config:~/c:ro" << "whole";
        QTest::newRow("an app's settings, to write") << ".config/gh:~/gh" << "to write in it";
        QTest::newRow("a colon, through a link") << "./colon:~/c" << "“:”";
        QTest::newRow("an invalid name") << "Data:~/data" << "invalid volume name";
        QTest::newRow("home as a name") << "home:~/data" << "invalid volume name";
        QTest::newRow("a relative target") << "data" << "absolute path";
        QTest::newRow("a target with ..") << "Clients/01:~/a/../../b" << "absolute path";
        QTest::newRow("home as target") << "Clients/01:~" << "not the home folder itself";
        QTest::newRow("a target outside home") << "Clients/01:/opt/project" << "not the home folder itself";
        QTest::newRow("a target with a comma") << "Clients/01:~/a,b" << "“,”";
        QTest::newRow("Z") << "Clients/01:~/p:Z" << "option Z";
        QTest::newRow("U") << "Clients/01:~/p:U" << "option U";
        QTest::newRow("another option") << "Clients/01:~/p:O" << "option O isn't supported";
        QTest::newRow("ro and rw") << "~/p:ro,rw" << "both ro and rw";
        QTest::newRow("too many parts") << "a:/b:ro:x" << "invalid volume";
        QTest::newRow("no folder") << ":/b" << "invalid volume";
    }

    void refuses()
    {
        QFETCH(QString, argument);
        QFETCH(QString, why);
        try {
            parseVolume(argument, home);
            QFAIL("shared");
        } catch (const WorkboxError &error) {
            QVERIFY2(error.message.contains(why), qPrintable(error.message));
        }
    }
};

QTEST_GUILESS_MAIN(VolumesTest)
#include "volumestest.moc"
