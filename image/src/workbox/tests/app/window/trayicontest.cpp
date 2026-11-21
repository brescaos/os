#include "mainwindow.h"
#include "trayicon.h"

#include <KStatusNotifierItem>

#include <QDir>
#include <QFile>
#include <QMenu>
#include <QTemporaryDir>
#include <QTest>

// The tray icon on the fake podman of fakes/, with the user's settings naming
// it.
class TrayIconTest : public QObject
{
    Q_OBJECT

    QTemporaryDir state;

    void writeState(const QString &name, const QByteArray &content)
    {
        QFile file(state.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(content);
    }

    static QByteArray container(const char *id, const char *name, bool isRunning)
    {
        return QStringLiteral("{\"Names\": [\"%1\"], \"State\": \"%2\", \"Labels\": {\"com.brescaos.workbox.name\": \"%3\", "
                              "\"com.brescaos.workbox.color\": \"#3584e4\"}}")
            .arg(QLatin1String(id), QLatin1String(isRunning ? "running" : "exited"), QLatin1String(name))
            .toUtf8();
    }

    bool hasCalled(const QString &command)
    {
        QFile file(state.filePath("calls"));
        return file.open(QIODevice::ReadOnly) && QString::fromUtf8(file.readAll()).split("\n\n").contains(command);
    }

    // The titles of its menu's entries.
    static QStringList entries(const QMenu *menu)
    {
        QStringList titles;
        for (const QAction *action : menu->actions()) {
            if (!action->isSeparator()) {
                titles.append(action->text());
            }
        }
        return titles;
    }

    static QMenu *submenu(const QMenu *menu, const QString &title)
    {
        for (const QAction *action : menu->actions()) {
            if (action->text() == title) {
                return action->menu();
            }
        }
        return nullptr;
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
        for (const QString &name : {"calls", "ps.json", "events"}) {
            QFile::remove(state.filePath(name));
        }
        writeState("config/workbox/workbox.conf", "[General]\nengine=podman\nimage=image\n");
        writeState("events", "");
        writeState("ps.json", "[" + container("c2", "Cliente 02", true) + ", " + container("c1", "Cliente 01", false) + "]");
    }

    // How many run: in the tooltip, and as a badge while any does.
    void runningCount()
    {
        WorkboxModel model;
        MainWindow window(model);
        TrayIcon trayIcon(model, window);
        QTRY_COMPARE(trayIcon.item()->toolTipSubTitle(), QStringLiteral("1 workbox running"));
        QVERIFY(!trayIcon.item()->overlayIconPixmap().isNull());

        model.stop("c2");
        QTRY_COMPARE(trayIcon.item()->toolTipSubTitle(), QStringLiteral("0 workboxes running"));
        QVERIFY(trayIcon.item()->overlayIconPixmap().isNull());
    }

    // A submenu per workbox, by id, and Quit; its Start starts it.
    void menu()
    {
        WorkboxModel model;
        MainWindow window(model);
        TrayIcon trayIcon(model, window);
        QMenu *menu = trayIcon.item()->contextMenu();
        QTRY_COMPARE(entries(menu), (QStringList{"Cliente 01", "Cliente 02", "Quit"}));
        QCOMPARE(entries(submenu(menu, "Cliente 01")), (QStringList{"Start", "Open a Terminal"}));
        QCOMPARE(entries(submenu(menu, "Cliente 02")), (QStringList{"Stop", "Open a Terminal"}));

        submenu(menu, "Cliente 01")->actions().first()->trigger();
        QTRY_VERIFY(hasCalled("start\nc1"));
        QTRY_COMPARE(entries(submenu(menu, "Cliente 01")), (QStringList{"Stop", "Open a Terminal"}));
    }

    void noWorkboxes()
    {
        writeState("ps.json", "[]");
        WorkboxModel model;
        MainWindow window(model);
        TrayIcon trayIcon(model, window);
        QMenu *menu = trayIcon.item()->contextMenu();
        QTRY_COMPARE(entries(menu), (QStringList{"No workboxes", "Quit"}));
        QVERIFY(!menu->actions().first()->isEnabled());
    }
};

QTEST_MAIN(TrayIconTest)
#include "trayicontest.moc"
