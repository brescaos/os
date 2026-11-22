#include "dialogs/volume/add.h"

#include <QCheckBox>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QTemporaryDir>
#include <QTest>

// The volume dialog in a home of its own, with a folder to share and one it
// refuses.
class AddVolumeDialogTest : public QObject
{
    Q_OBJECT

    QTemporaryDir directory;
    QString home;

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
        home = QDir(directory.path()).canonicalPath() + "/home";
        QVERIFY(QDir(home).mkpath("Clients/02"));
        QVERIFY(QDir(home).mkpath(".ssh"));
        qputenv("HOME", QFile::encodeName(home));
    }

    // A shared folder's place in the home, while the workbox's is empty.
    void fillsTheTarget()
    {
        AddVolumeDialog dialog;
        auto *folderEdit = child<QLineEdit>(dialog, "folderEdit");
        auto *targetEdit = child<QLineEdit>(dialog, "targetEdit");
        folderEdit->setText("Clients/02");
        Q_EMIT folderEdit->editingFinished();
        QCOMPARE(targetEdit->text(), QStringLiteral("~/Clients/02"));
        targetEdit->setText("~/project");
        folderEdit->setText("~/Clients");
        Q_EMIT folderEdit->editingFinished();
        QCOMPARE(targetEdit->text(), QStringLiteral("~/project"));
    }

    void shares()
    {
        AddVolumeDialog dialog;
        auto *okButton = child<QPushButton>(dialog, "okButton");
        QVERIFY(!okButton->isEnabled());
        child<QLineEdit>(dialog, "folderEdit")->setText("~/Clients/02");
        child<QLineEdit>(dialog, "targetEdit")->setText("~/project");
        child<QCheckBox>(dialog, "readOnlyCheck")->setChecked(true);
        QVERIFY(okButton->isEnabled());
        QCOMPARE(dialog.volume(), (Volume{.source = home + "/Clients/02", .target = home + "/project", .isReadOnly = true}));
    }

    // Why, in place of the hint.
    void refusesABlockedFolder()
    {
        AddVolumeDialog dialog;
        child<QLineEdit>(dialog, "folderEdit")->setText("~/.ssh");
        child<QLineEdit>(dialog, "targetEdit")->setText("~/keys");
        QVERIFY(child<QLabel>(dialog, "hintLabel")->text().contains("it holds keys"));
        QVERIFY(!child<QPushButton>(dialog, "okButton")->isEnabled());
    }

    // A named one, without read-only.
    void names()
    {
        AddVolumeDialog dialog;
        dialog.show();
        child<QRadioButton>(dialog, "namedButton")->setChecked(true);
        QVERIFY(!child<QCheckBox>(dialog, "readOnlyCheck")->isVisible());
        child<QLineEdit>(dialog, "nameEdit")->setText("Cache");
        child<QLineEdit>(dialog, "targetEdit")->setText("~/.cache");
        QVERIFY(child<QLabel>(dialog, "hintLabel")->text().startsWith("invalid volume name Cache"));
        child<QLineEdit>(dialog, "nameEdit")->setText("cache");
        QVERIFY(child<QPushButton>(dialog, "okButton")->isEnabled());
        QCOMPARE(dialog.volume(), (Volume{.name = "cache", .target = home + "/.cache"}));
    }

    // Its fields as it is, with ~ for the home.
    void edits()
    {
        AddVolumeDialog dialog(Volume{.source = home + "/Clients/02", .target = home + "/project", .isReadOnly = true});
        QCOMPARE(dialog.windowTitle(), QStringLiteral("Edit Volume"));
        QVERIFY(child<QRadioButton>(dialog, "sharedButton")->isChecked());
        QCOMPARE(child<QLineEdit>(dialog, "folderEdit")->text(), QStringLiteral("~/Clients/02"));
        QCOMPARE(child<QLineEdit>(dialog, "targetEdit")->text(), QStringLiteral("~/project"));
        QVERIFY(child<QCheckBox>(dialog, "readOnlyCheck")->isChecked());

        AddVolumeDialog unnamed(Volume{.target = home + "/tmp"});
        QVERIFY(child<QRadioButton>(unnamed, "unnamedButton")->isChecked());
        QCOMPARE(unnamed.volume(), (Volume{.target = home + "/tmp"}));
    }
};

QTEST_MAIN(AddVolumeDialogTest)
#include "addtest.moc"
