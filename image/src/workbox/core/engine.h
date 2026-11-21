// What workbox needs from a container engine (README.md), in workbox's own
// terms: only an engine's implementation knows its commands. Each method
// throws a WorkboxError when the engine fails.
#pragma once

#include "workbox.h"

#include <QList>
#include <QMap>

class Engine
{
public:
    virtual ~Engine() = default;

    virtual QList<Workbox> list() = 0;

    // Creates the workbox from image, with its home volume and, with
    // hasSudo, sudo without a password for the user. Leaves it stopped, and
    // nothing of it when it fails.
    virtual void create(const Workbox &workbox, const QString &image) = 0;

    virtual void start(const QString &id) = 0;

    virtual void stop(const QString &id) = 0;

    // Runs a login shell in the running workbox, as the user in their home
    // or, with asRoot, as root in /root, on this process's terminal, with
    // environment added to the shell's, until it exits; returns its exit
    // status.
    virtual int enter(const QString &id, const QMap<QString, QString> &environment, bool asRoot) = 0;

    // Stops the workbox if it runs, and removes it and its home volume.
    virtual void remove(const QString &id) = 0;
};
