#include "mainwindow.h"
#include "terminals.h"

#include <QDialog>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>

// The window on the fake podman of fakes/, with the user's settings naming
// it.
class MainWindowTest : public QObject
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
        return QStringLiteral("{\"Names\": [\"%1\"], \"State\": \"%2\", \"Image\": \"image\", \"Labels\": {\"com.brescaos.workbox.name\": "
                              "\"%3\", \"com.brescaos.workbox.color\": \"#3584e4\"}}")
            .arg(QLatin1String(id), QLatin1String(isRunning ? "running" : "exited"), QLatin1String(name))
            .toUtf8();
    }

    bool hasCalled(const QString &command)
    {
        QFile file(state.filePath("calls"));
        return file.open(QIODevice::ReadOnly) && QString::fromUtf8(file.readAll()).split("\n\n").contains(command);
    }

    template<typename T>
    static T *child(const QObject &parent, const QString &name)
    {
        T *found = parent.findChild<T *>(name);
        if (!found) {
            qWarning() << "no" << name;
        }
        return found;
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
        writeState("ps.json", "[" + container("c2", "Cliente 02", true) + ", " + container("c1", "Cliente 01", false) + "]");
    }

    // Every workbox, the first one selected, with its settings.
    void showsTheFirst()
    {
        WorkboxModel model;
        MainWindow window(model);
        auto *list = child<QListView>(window, "workboxList");
        QTRY_COMPARE(list->model()->rowCount(), 2);
        QTRY_COMPARE(child<QLabel>(window, "nameLabel")->text(), QStringLiteral("Cliente 01"));
        QVERIFY(child<QLabel>(window, "detailLabel")->text().startsWith("c1 · "));
        QCOMPARE(child<QLabel>(window, "countLabel")->text(), QStringLiteral("1 of 2 running"));
        QCOMPARE(child<QLabel>(window, "volumesLabel")->text(), QStringLiteral("None"));
        // Whole, on one line; the color as a swatch, its value in the tooltip.
        auto *homeLabel = child<QLabel>(window, "homeLabel");
        QCOMPARE(homeLabel->text(), QStringLiteral("workbox-c1.home"));
        QVERIFY(!homeLabel->wordWrap());
        auto *colorLabel = child<QLabel>(window, "colorLabel");
        QVERIFY(!colorLabel->pixmap().isNull());
        QCOMPARE(colorLabel->toolTip(), QStringLiteral("#3584e4"));
        QVERIFY(child<QPushButton>(window, "createButton")->isEnabled());
    }

    // Starting it from its stopped page starts it, and the page goes.
    void start()
    {
        WorkboxModel model;
        MainWindow window(model);
        window.show();
        auto *startButton = child<QPushButton>(window, "startAndOpenButton");
        QTRY_VERIFY(startButton->isVisible());
        startButton->click();
        QTRY_VERIFY(hasCalled("start\nc1"));
        QTRY_VERIFY(!child<QWidget>(window, "stoppedPage")->isVisible());
    }

    void stop()
    {
        WorkboxModel model;
        MainWindow window(model);
        window.show();
        auto *list = child<QListView>(window, "workboxList");
        QTRY_COMPARE(list->model()->rowCount(), 2);
        list->setCurrentIndex(list->model()->index(1, 0));
        auto *stopButton = child<QPushButton>(window, "stopButton");
        QTRY_VERIFY(stopButton->isVisible());
        stopButton->click();
        QTRY_VERIFY(hasCalled("stop\n--time\n0\nc2"));
        QTRY_VERIFY(child<QPushButton>(window, "startButton")->isVisible());
    }

    // From the tray: a stopped one is selected, on its terminals, and
    // started.
    void openTerminal()
    {
        WorkboxModel model;
        MainWindow window(model);
        window.show();
        auto *list = child<QListView>(window, "workboxList");
        QTRY_COMPARE(list->model()->rowCount(), 2);
        list->setCurrentIndex(list->model()->index(1, 0));
        QTRY_COMPARE(child<QLabel>(window, "nameLabel")->text(), QStringLiteral("Cliente 02"));
        window.openTerminal("c1");
        QCOMPARE(child<QLabel>(window, "nameLabel")->text(), QStringLiteral("Cliente 01"));
        QTRY_VERIFY(hasCalled("start\nc1"));
    }

    // Only once its id is typed.
    void remove()
    {
        WorkboxModel model;
        MainWindow window(model);
        QTRY_COMPARE(child<QLabel>(window, "nameLabel")->text(), QStringLiteral("Cliente 01"));
        child<QPushButton>(window, "removeButton")->click();
        auto *dialog = child<QDialog>(window, "removeDialog");
        QVERIFY(dialog);
        auto *removeButton = child<QPushButton>(*dialog, "confirmRemoveButton");
        auto *confirmationEdit = child<QLineEdit>(*dialog, "confirmationEdit");
        QVERIFY(!removeButton->isEnabled());
        confirmationEdit->setText("c");
        QVERIFY(!removeButton->isEnabled());
        confirmationEdit->setText("c1");
        QVERIFY(removeButton->isEnabled());
        removeButton->click();
        QTRY_VERIFY(hasCalled("rm\n--force\n--time\n0\n--volumes\nc1"));
        QTRY_COMPARE(child<QLabel>(window, "nameLabel")->text(), QStringLiteral("Cliente 02"));
    }

    void searches()
    {
        WorkboxModel model;
        MainWindow window(model);
        auto *list = child<QListView>(window, "workboxList");
        QTRY_COMPARE(list->model()->rowCount(), 2);
        child<QLineEdit>(window, "searchEdit")->setText("02");
        QCOMPARE(list->model()->rowCount(), 1);
        child<QLineEdit>(window, "searchEdit")->setText("C1");
        QCOMPARE(list->model()->rowCount(), 1);
        QCOMPARE(list->model()->index(0, 0).data(WorkboxModel::IdRole).toString(), QStringLiteral("c1"));
    }

    // No workboxes: how to create one.
    void empty()
    {
        writeState("ps.json", "[]");
        WorkboxModel model;
        MainWindow window(model);
        window.show();
        QTRY_VERIFY(child<QWidget>(window, "emptyPage")->isVisible());
        child<QPushButton>(window, "emptyCreateButton")->click();
        QVERIFY(child<QDialog>(window, "createDialog"));
    }

    // Without settings, why.
    void problem()
    {
        QFile::remove(state.filePath("config/workbox/workbox.conf"));
        WorkboxModel model;
        MainWindow window(model);
        window.show();
        QVERIFY(child<QLabel>(window, "problemLabel")->isVisible());
        QVERIFY(child<QLabel>(window, "problemLabel")->text().startsWith("no engine in "));
        QVERIFY(!child<QPushButton>(window, "createButton")->isEnabled());
    }

    // sh sets the colors, then runs the command line as it is.
    void commandLineWithColors()
    {
        const QStringList commandLine = ::commandLineWithColors(QColor("#3584e4"), {"podman", "exec", "$(id)"});
        QCOMPARE(commandLine.mid(0, 4), (QStringList{"/bin/sh", "-c", "printf '%s' \"$1\"; shift; exec \"$@\"", "sh"}));
        QVERIFY(commandLine[4].startsWith("\x1b]11;#"));
        QCOMPARE(commandLine.mid(5), (QStringList{"podman", "exec", "$(id)"}));
    }
};

QTEST_MAIN(MainWindowTest)
#include "mainwindowtest.moc"
