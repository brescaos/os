#include "dialogs/workbox/create.h"

#include "color.h"
#include "dialogs/volume/add.h"

#include <KMessageWidget>

#include <QCheckBox>
#include <QColorDialog>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QToolButton>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>

// The create dialog on the fake podman of fakes/, with the user's settings
// naming it, and a home of its own.
class CreateWorkboxDialogTest : public QObject
{
    Q_OBJECT

    QTemporaryDir state;

    void writeState(const QString &name, const QByteArray &content)
    {
        QFile file(state.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(content);
    }

    QString calls()
    {
        QFile file(state.filePath("calls"));
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
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

    // The last row's field.
    template<typename T>
    static T *lastOf(const QObject &parent, const QString &name)
    {
        const QList<T *> found = parent.findChildren<T *>(name);
        return found.isEmpty() ? nullptr : found.last();
    }

    // Through the volume dialog: its type, then its source (the folder or
    // the name) and the folder in the workbox.
    static void addVolume(QWidget &dialog, const QString &kindButton, const QString &source, const QString &target, bool isReadOnly = false)
    {
        child<QPushButton>(dialog, "addVolumeButton")->click();
        auto *volumeDialog = lastOf<AddVolumeDialog>(dialog, "volumeDialog");
        QVERIFY(volumeDialog);
        child<QRadioButton>(*volumeDialog, kindButton)->setChecked(true);
        if (kindButton == "sharedButton") {
            child<QLineEdit>(*volumeDialog, "folderEdit")->setText(source);
            child<QCheckBox>(*volumeDialog, "readOnlyCheck")->setChecked(isReadOnly);
        } else if (kindButton == "namedButton") {
            child<QLineEdit>(*volumeDialog, "nameEdit")->setText(source);
        }
        child<QLineEdit>(*volumeDialog, "targetEdit")->setText(target);
        auto *okButton = child<QPushButton>(*volumeDialog, "okButton");
        QVERIFY(okButton->isEnabled());
        okButton->click();
    }

    static QStringList mappings(const QWidget &dialog)
    {
        QStringList mappings;
        for (const QLabel *label : dialog.findChildren<QLabel *>("volumeMappingLabel")) {
            mappings.append(label->text());
        }
        return mappings;
    }

private Q_SLOTS:
    void initTestCase()
    {
        qputenv("PATH", FAKES_DIR ":" + qgetenv("PATH"));
        qputenv("FAKE_STATE", QFile::encodeName(state.path()));
        qputenv("XDG_CONFIG_HOME", QFile::encodeName(state.filePath("config")));
        qputenv("HOME", QFile::encodeName(state.filePath("home")));
        QVERIFY(QDir(state.path()).mkpath("config/workbox"));
        QVERIFY(QDir(state.path()).mkpath("home/Projects"));
    }

    void init()
    {
        for (const QString &name : {"calls", "fail", "delay"}) {
            QFile::remove(state.filePath(name));
        }
        writeState("config/workbox/workbox.conf", "[General]\nengine=podman\nimage=image\n");
        writeState("events", "");
        writeState("ps.json",
                   "[{\"Names\": [\"c1\"], \"State\": \"exited\", \"Image\": \"image\", \"Labels\": {\"com.brescaos.workbox.name\": "
                   "\"Cliente 01\", \"com.brescaos.workbox.color\": \"#3584e4\"}}]");
    }

    // Until one is typed, the id is the name in its characters.
    void idFollowsName()
    {
        WorkboxModel model;
        CreateWorkboxDialog dialog(model);
        auto *idEdit = child<QLineEdit>(dialog, "idEdit");
        child<QLineEdit>(dialog, "nameEdit")->setText("Clíent 03 / Web");
        QCOMPARE(idEdit->text(), QStringLiteral("client-03-web"));
        QCOMPARE(child<QLabel>(dialog, "titleLabel")->text(), QStringLiteral("Clíent 03 / Web"));
        idEdit->clear();
        QTest::keyClicks(idEdit, "web");
        child<QLineEdit>(dialog, "nameEdit")->setText("Other");
        QCOMPARE(idEdit->text(), QStringLiteral("web"));
        QVERIFY(child<QPushButton>(dialog, "createWorkboxButton")->isEnabled());
    }

    void refusesTakenId()
    {
        WorkboxModel model;
        QTRY_COMPARE(model.rowCount(), 1);
        CreateWorkboxDialog dialog(model);
        child<QLineEdit>(dialog, "nameEdit")->setText("C1");
        QCOMPARE(child<QLabel>(dialog, "idHintLabel")->text(), QStringLiteral("There's already a workbox with this id."));
        QVERIFY(!child<QPushButton>(dialog, "createWorkboxButton")->isEnabled());
    }

    // Create's pick first; a suggestion, what it would pick after it, or the
    // color dialog's changes it.
    void suggestsColors()
    {
        WorkboxModel model;
        QTRY_COMPARE(model.rowCount(), 1);
        CreateWorkboxDialog dialog(model);
        dialog.show();
        const QList<QColor> suggested = WorkboxColor::suggestions({QColor("#3584e4")}, 5);
        auto *colorSwatch = child<QLabel>(dialog, "colorSwatch");
        QCOMPARE(colorSwatch->accessibleName(), WorkboxColor::pickFromPalette({QColor("#3584e4")}).name());
        const QList<QToolButton *> buttons = dialog.findChildren<QToolButton *>();
        for (const QColor &color : suggested) {
            const auto button = std::ranges::find_if(buttons, [&color](const QToolButton *button) {
                return button->accessibleName() == color.name();
            });
            QVERIFY(button != buttons.end());
            (*button)->click();
            QCOMPARE(colorSwatch->accessibleName(), color.name());
        }

        child<QPushButton>(dialog, "changeColorButton")->click();
        auto *colorDialog = child<QColorDialog>(dialog, "colorDialog");
        QVERIFY(colorDialog);
        QCOMPARE(colorDialog->currentColor().name(), suggested.last().name());
        colorDialog->setCurrentColor(QColor("#123456"));
        colorDialog->accept();
        QCOMPARE(colorSwatch->accessibleName(), QStringLiteral("#123456"));
        child<QLineEdit>(dialog, "nameEdit")->setText("Cliente 05");
        child<QPushButton>(dialog, "createWorkboxButton")->click();
        QTRY_VERIFY(child<QWidget>(dialog, "donePage")->isVisible());
        QVERIFY(calls().contains("--label\ncom.brescaos.workbox.color=#123456\n"));
    }

    // Each from the volume dialog, in a row to edit or remove it.
    void listsVolumes()
    {
        WorkboxModel model;
        CreateWorkboxDialog dialog(model);
        dialog.show();
        child<QLineEdit>(dialog, "nameEdit")->setText("Cliente 02");
        QVERIFY(child<QLabel>(dialog, "noVolumesLabel")->isVisible());
        addVolume(dialog, "sharedButton", "~/Projects", "~/project");
        addVolume(dialog, "unnamedButton", QString(), "~/tmp");
        QVERIFY(!child<QLabel>(dialog, "noVolumesLabel")->isVisible());
        QCOMPARE(mappings(dialog), (QStringList{"~/Projects → ~/project", "~/tmp"}));
        QCOMPARE(child<QLabel>(dialog, "footerNote")->text(), QStringLiteral("It shares 1 folder with your system."));

        dialog.findChildren<QToolButton *>("editVolumeButton").first()->click();
        auto *volumeDialog = lastOf<AddVolumeDialog>(dialog, "volumeDialog");
        QCOMPARE(child<QLineEdit>(*volumeDialog, "folderEdit")->text(), QStringLiteral("~/Projects"));
        child<QLineEdit>(*volumeDialog, "targetEdit")->setText("~/work");
        child<QPushButton>(*volumeDialog, "okButton")->click();
        QCOMPARE(mappings(dialog), (QStringList{"~/Projects → ~/work", "~/tmp"}));

        dialog.findChildren<QToolButton *>("removeVolumeButton").first()->click();
        QTRY_COMPARE(mappings(dialog), (QStringList{"~/tmp"}));
        QCOMPARE(child<QLabel>(dialog, "footerNote")->text(), QStringLiteral("It shares no folder with your system."));
    }

    // As workbox create, then its row is in the model at once.
    void creates()
    {
        WorkboxModel model;
        QTRY_COMPARE(model.rowCount(), 1);
        CreateWorkboxDialog dialog(model);
        dialog.show();
        QSignalSpy created(&dialog, &CreateWorkboxDialog::created);
        child<QLineEdit>(dialog, "nameEdit")->setText("Cliente 03");
        addVolume(dialog, "sharedButton", "~/Projects", "~/project", true);
        addVolume(dialog, "namedButton", "cache", "~/.cache");
        addVolume(dialog, "unnamedButton", QString(), "~/tmp");
        child<QCheckBox>(dialog, "sudoCheck")->setChecked(true);
        child<QPushButton>(dialog, "createWorkboxButton")->click();

        QTRY_VERIFY(child<QWidget>(dialog, "donePage")->isVisible());
        QCOMPARE(created.count(), 1);
        QCOMPARE(created.first().first().toString(), QStringLiteral("cliente-03"));
        QCOMPARE(child<QLabel>(dialog, "doneTitle")->text(), QStringLiteral("“Cliente 03” is ready"));
        const QString home = state.filePath("home");
        QVERIFY(calls().contains("create\n--name\ncliente-03\n"));
        QVERIFY(calls().contains("--volume\n" + home + "/Projects:" + home + "/project:z,ro\n"));
        QVERIFY(calls().contains("--volume\nworkbox-cliente-03.cache:" + home + "/.cache\n"));
        QVERIFY(calls().contains("--volume\n" + home + "/tmp\n"));
        QVERIFY(calls().contains("start\ncliente-03\n"));
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.index(1).data(WorkboxModel::IdRole).toString(), QStringLiteral("cliente-03"));
    }

    // Back to the form, with why.
    void showsFailure()
    {
        writeState("fail", "create\n");
        WorkboxModel model;
        QTRY_COMPARE(model.rowCount(), 1);
        CreateWorkboxDialog dialog(model);
        dialog.show();
        child<QLineEdit>(dialog, "nameEdit")->setText("Cliente 04");
        child<QPushButton>(dialog, "createWorkboxButton")->click();
        auto *errorMessage = child<KMessageWidget>(dialog, "errorMessage");
        QTRY_VERIFY(errorMessage->isVisible());
        QVERIFY(errorMessage->text().startsWith("podman create --name cliente-04 "));
        QVERIFY(child<QWidget>(dialog, "formPage")->isVisible());
        QCOMPARE(model.rowCount(), 1);
    }
};

QTEST_MAIN(CreateWorkboxDialogTest)
#include "createtest.moc"
