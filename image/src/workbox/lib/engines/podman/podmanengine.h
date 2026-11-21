// The Engine on rootless Podman (README.md): the only code that knows its
// commands. It runs `podman` from PATH, as PodmanJobs.
#pragma once

#include "engine.h"

class PodmanEngine : public Engine
{
public:
    ListJob *list() override;
    WatchJob *watch() override;
    EngineJob *create(const Workbox &workbox, const QString &image) override;
    EngineJob *start(const QString &id) override;
    EngineJob *stop(const QString &id) override;
    EngineJob *remove(const Workbox &workbox) override;
    EnterJob *enter(const QString &id, const QStringList &command, const QMap<QString, QString> &environment, bool asRoot) override;
    QStringList shellCommandLine(const QString &id, const QMap<QString, QString> &environment) override;
};
