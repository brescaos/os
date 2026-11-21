// What the commands do, on any engine (README.md), for the command line and
// any other interface: create as a job, which never blocks, and what follows
// from a list of workboxes.
#pragma once

#include "engine.h"

#include <optional>

namespace Commands
{

// Creates the workbox id from image: lists the workboxes, then creates it.
// Without a name, its name is its id; without a color (#rgb, #rrggbb or
// r,g,b), the palette's farthest from the others'. With hasSudo, the user can
// use sudo in it. volumes are -v's, "[folder|name:]target[:options]"
// (volumes.h).
// Fails without creating anything when an argument is wrong or the id is
// taken. Its output() is the engine's.
class CreateJob : public EngineJob
{
    Q_OBJECT

public:
    CreateJob(Engine &engine,
              const QString &id,
              const std::optional<QString> &name,
              const std::optional<QString> &color,
              bool hasSudo,
              const QStringList &volumes,
              const QString &image,
              QObject *parent = nullptr);

    void start() override;

    // The workbox it created, once it has succeeded.
    Workbox workbox() const
    {
        return m_workbox;
    }

protected:
    bool doKill() override;

private:
    void createFrom(const QList<Workbox> &workboxes);
    // Runs job, an engine's step of this one, with its output as this one's;
    // on success, then().
    void runStep(EngineJob *job, const std::function<void()> &then);

    Engine &m_engine;
    std::optional<QString> m_name;
    std::optional<QString> m_color;
    QStringList m_volumes;
    QString m_image;
    Workbox m_workbox;
    EngineJob *m_step = nullptr;
};

// The workbox id among workboxes; throws a WorkboxError when it isn't there.
Workbox find(const QList<Workbox> &workboxes, const QString &id);

QList<Workbox> sortedById(QList<Workbox> workboxes);

}
