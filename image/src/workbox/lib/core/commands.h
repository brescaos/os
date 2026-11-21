// What the commands do, on any engine (README.md), for the command line and
// any other interface: create as a job, which never blocks, and what follows
// from a list of workboxes.
#pragma once

#include "engine.h"

#include <optional>

namespace Commands
{

// What create takes besides the id, as the user gives it.
struct CreateOptions {
    // Without it, its id.
    std::optional<QString> name;
    // #rgb, #rrggbb or r,g,b; without it, the palette's farthest from the
    // others'.
    std::optional<QString> color;
    // The user can use sudo in it.
    bool hasSudo = false;
    // -v's, "[folder|name:]target[:options]" (volumes.h).
    QStringList volumes;
    // Its apps can show windows in the user's Wayland session: there must be
    // one (WAYLAND_DISPLAY).
    bool hasWayland = false;
};

// Creates the workbox id from image: lists the workboxes, then creates it.
// Fails without creating anything when an option is wrong or the id is
// taken. Its output() is the engine's.
class CreateJob : public EngineJob
{
    Q_OBJECT

public:
    CreateJob(Engine &engine, const QString &id, const CreateOptions &options, const QString &image, QObject *parent = nullptr);

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
    CreateOptions m_options;
    QString m_image;
    Workbox m_workbox;
    EngineJob *m_step = nullptr;
};

// The workbox id among workboxes; throws a WorkboxError when it isn't there.
Workbox find(const QList<Workbox> &workboxes, const QString &id);

QList<Workbox> sortedById(QList<Workbox> workboxes);

}
