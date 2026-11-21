// The Engine on rootless Podman (README.md): the only code that knows its
// commands. It runs `podman` from PATH, and its errors go to stderr.
#pragma once

#include "engine.h"

class PodmanEngine : public Engine
{
public:
    QList<Workbox> list() override;
    void create(const Workbox &workbox, const QString &image) override;
    void start(const QString &id) override;
    void stop(const QString &id) override;
    int enter(const QString &id, const QMap<QString, QString> &environment, bool asRoot) override;
    void remove(const QString &id) override;
};
