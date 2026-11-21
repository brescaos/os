// What workbox needs from a container engine (README.md), in workbox's own
// terms: only an engine's implementation knows its commands. Each method
// throws a WorkboxError when the engine fails.
#pragma once

#include "workbox.h"

#include <QList>

class Engine
{
public:
    virtual ~Engine() = default;

    virtual QList<Workbox> list() = 0;

    // Creates the workbox from image, with its home volume.
    virtual void create(const Workbox &workbox, const QString &image) = 0;

    // Stops the workbox if it runs, and removes it and its home volume.
    virtual void remove(const QString &id) = 0;
};
