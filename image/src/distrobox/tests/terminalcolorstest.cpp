#include "terminalcolors.h"

#include <QTest>

#include <cmath>

using namespace TerminalColors;

class TerminalColorsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void enteredNameFollowsDistrobox_data()
    {
        QTest::addColumn<QStringList>("arguments");
        QTest::addColumn<QString>("nameFromEnvironment");
        QTest::addColumn<QString>("name");

        QTest::newRow("argument") << QStringList{"dev"} << "" << "dev";
        QTest::newRow("-n") << QStringList{"-n", "dev"} << "" << "dev";
        QTest::newRow("--name") << QStringList{"--name", "dev"} << "" << "dev";
        QTest::newRow("after flags") << QStringList{"--root", "-nw", "dev"} << "" << "dev";
        QTest::newRow("after -a's value") << QStringList{"-a", "--env X=1", "dev"} << "" << "dev";
        QTest::newRow("before a command") << QStringList{"dev", "--", "bash"} << "" << "dev";
        QTest::newRow("in a command") << QStringList{"-e", "dev"} << "" << "my-distrobox";
        QTest::newRow("from the environment") << QStringList{} << "box" << "box";
        QTest::newRow("argument over environment") << QStringList{"dev"} << "box" << "dev";
        QTest::newRow("Distrobox's default") << QStringList{} << "" << "my-distrobox";
    }

    void enteredNameFollowsDistrobox()
    {
        QFETCH(QStringList, arguments);
        QFETCH(QString, nameFromEnvironment);
        QFETCH(QString, name);
        QCOMPARE(enteredName(arguments, nameFromEnvironment), name);
    }

    // Konsole's HSV ranges, before the accent.
    void konsoleTabColorRanges()
    {
        for (const QString name : {"dev", "test2", "fedora", "my-distrobox"}) {
            const QColor color = konsoleTabColor(name, QColor());
            QVERIFY2(color.hsvSaturation() >= 130 && color.hsvSaturation() < 220, qPrintable(name));
            QVERIFY2(color.value() >= 170 && color.value() < 230, qPrintable(name));
            QCOMPARE(konsoleTabColor(name, QColor()), color);
        }
        QVERIFY(konsoleTabColor("dev", QColor()) != konsoleTabColor("fedora", QColor()));
    }

    // A quarter accent, three quarters Konsole's color, as integers.
    void konsoleTabColorAccent()
    {
        const QColor color = konsoleTabColor("dev", QColor());
        QCOMPARE(konsoleTabColor("dev", Qt::black), QColor(color.red() * 3 / 4, color.green() * 3 / 4, color.blue() * 3 / 4));
    }

    void oklchHueOfPrimaries()
    {
        QCOMPARE(qRound(oklchHue(QColor(255, 0, 0))), 29);
        QCOMPARE(qRound(oklchHue(QColor(0, 255, 0))), 142);
        QCOMPARE(qRound(oklchHue(QColor(0, 0, 255))), 264);
    }

    // Every hue: AAA (7:1) for the text (#fcfcfc, profile.sh) and the label on
    // the background, and both keep the hue.
    void forEveryHue()
    {
        const QColor text(0xfc, 0xfc, 0xfc);
        for (double hue = 0; hue < 360; hue += 0.5) {
            const Colors colors = forHue(hue);
            const QByteArray message = QByteArray::number(hue);
            QVERIFY2(contrastRatio(text, colors.background) >= 7, message);
            QVERIFY2(contrastRatio(colors.label, colors.background) >= 7, message);
            for (const QColor &color : {colors.background, colors.label}) {
                const double difference = std::abs(oklchHue(color) - hue);
                QVERIFY2(std::min(difference, 360 - difference) < 3, message);
            }
        }
    }

    void contrastRatioExtremes()
    {
        QCOMPARE(contrastRatio(Qt::black, Qt::white), 21.0);
        QCOMPARE(contrastRatio(Qt::white, Qt::white), 1.0);
    }
};

QTEST_GUILESS_MAIN(TerminalColorsTest)
#include "terminalcolorstest.moc"
