#include "color.h"
#include "commands.h"

#include <QTest>

namespace
{

const QString image = QStringLiteral("registry.fedoraproject.org/fedora-toolbox:44");

// The workboxes in memory, and the image each one was created from.
class FakeEngine : public Engine
{
public:
    QList<Workbox> workboxes;
    QStringList images;

    QList<Workbox> list() override
    {
        return workboxes;
    }

    void create(const Workbox &workbox, const QString &image) override
    {
        workboxes.append(workbox);
        images.append(image);
    }

    void remove(const QString &id) override
    {
        workboxes.removeIf([&id](const Workbox &workbox) {
            return workbox.id == id;
        });
    }
};

}

class CommandsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void createWithNameAndColor()
    {
        FakeEngine engine;
        const Workbox workbox = Commands::create(engine, "c1", QStringLiteral("Cliente 01"), QStringLiteral("53,132,228"), image);
        QCOMPARE(workbox.id, QStringLiteral("c1"));
        QCOMPARE(workbox.name, QStringLiteral("Cliente 01"));
        QCOMPARE(workbox.color.name(), QStringLiteral("#3584e4"));
        QCOMPARE(engine.workboxes.size(), 1);
        QCOMPARE(engine.images, QStringList{image});
    }

    void createDefaultsToItsIdAndThePalette()
    {
        FakeEngine engine;
        const Workbox first = Commands::create(engine, "c1", std::nullopt, std::nullopt, image);
        QCOMPARE(first.name, QStringLiteral("c1"));
        QCOMPARE(first.color, WorkboxColor::palette().first());

        const Workbox second = Commands::create(engine, "c2", std::nullopt, std::nullopt, image);
        QCOMPARE(second.color, WorkboxColor::pickFromPalette({first.color}));
    }

    void createRefuses_data()
    {
        QTest::addColumn<QString>("id");
        QTest::addColumn<std::optional<QString>>("name");
        QTest::addColumn<std::optional<QString>>("color");
        QTest::addColumn<QString>("message");

        const std::optional<QString> none;
        QTest::newRow("invalid id") << "C1" << none << none << "invalid id \"C1\"";
        QTest::newRow("existing id") << "c0" << none << none << "a workbox with the id \"c0\" already exists";
        QTest::newRow("empty name") << "c1" << std::optional<QString>(" ") << none << "the name can't be empty";
        QTest::newRow("invalid color") << "c1" << none << std::optional<QString>("red") << "invalid color \"red\"";
    }

    void createRefuses()
    {
        QFETCH(QString, id);
        QFETCH(std::optional<QString>, name);
        QFETCH(std::optional<QString>, color);
        QFETCH(QString, message);

        FakeEngine engine;
        engine.workboxes.append({.id = "c0", .name = "c0", .color = Qt::red});
        try {
            Commands::create(engine, id, name, color, image);
            QFAIL("created");
        } catch (const WorkboxError &error) {
            QVERIFY2(error.message.startsWith(message), qPrintable(error.message));
        }
        QCOMPARE(engine.workboxes.size(), 1);
    }

    void find()
    {
        FakeEngine engine;
        engine.workboxes.append({.id = "c1", .name = "Cliente 01", .color = Qt::red});
        QCOMPARE(Commands::find(engine, "c1").name, QStringLiteral("Cliente 01"));
        QVERIFY_THROWS_EXCEPTION(WorkboxError, Commands::find(engine, "c2"));
    }

    void listIsById()
    {
        FakeEngine engine;
        for (const QString id : {"c2", "a1", "c10"}) {
            engine.workboxes.append({.id = id, .name = id, .color = Qt::red});
        }
        QStringList ids;
        for (const Workbox &workbox : Commands::list(engine)) {
            ids.append(workbox.id);
        }
        QCOMPARE(ids, (QStringList{"a1", "c10", "c2"}));
    }
};

QTEST_GUILESS_MAIN(CommandsTest)
#include "commandstest.moc"
