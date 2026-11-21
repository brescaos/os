#include "workbox.h"

#include <QTest>

class WorkboxTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void validIds_data()
    {
        QTest::addColumn<QString>("id");
        QTest::addColumn<bool>("isValid");

        QTest::newRow("letters and digits") << "c1" << true;
        QTest::newRow("starts with a digit") << "1c" << true;
        QTest::newRow("hyphens") << "client-01-dev" << true;
        QTest::newRow("32 characters") << QString(32, QLatin1Char('a')) << true;
        QTest::newRow("33 characters") << QString(33, QLatin1Char('a')) << false;
        QTest::newRow("empty") << "" << false;
        QTest::newRow("starts with a hyphen") << "-c1" << false;
        QTest::newRow("uppercase") << "C1" << false;
        QTest::newRow("underscore") << "c_1" << false;
        QTest::newRow("dot") << "c.1" << false;
        QTest::newRow("space") << "c 1" << false;
        QTest::newRow("accent") << "cliént" << false;
    }

    void validIds()
    {
        QFETCH(QString, id);
        QFETCH(bool, isValid);
        QCOMPARE(isValidId(id), isValid);
    }

    void homeVolumeName()
    {
        QCOMPARE(::homeVolumeName("c1"), QStringLiteral("workbox-c1-home"));
    }

    void labelsKeepNameAndColor()
    {
        const Workbox workbox{.id = "c1", .name = "Cliente 01", .color = QColor("#3584e4")};
        const QMap<QString, QString> labels = labelsOf(workbox);
        QCOMPARE(labels.value("com.brescaos.workbox.name"), QStringLiteral("Cliente 01"));
        QCOMPARE(labels.value("com.brescaos.workbox.color"), QStringLiteral("#3584e4"));

        const Workbox read = workboxFromLabels("c1", labels);
        QCOMPARE(read.id, workbox.id);
        QCOMPARE(read.name, workbox.name);
        QCOMPARE(read.color, workbox.color);
        QVERIFY(!read.hasSudo);
        QVERIFY(!labels.contains("com.brescaos.workbox.sudo"));
    }

    void labelsKeepSudo()
    {
        const Workbox workbox{.id = "c1", .name = "c1", .color = Qt::red, .hasSudo = true};
        QCOMPARE(labelsOf(workbox).value("com.brescaos.workbox.sudo"), QStringLiteral("true"));
        QVERIFY(workboxFromLabels("c1", labelsOf(workbox)).hasSudo);
    }
};

QTEST_GUILESS_MAIN(WorkboxTest)
#include "workboxtest.moc"
