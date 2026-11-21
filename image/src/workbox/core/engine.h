// What workbox needs from a container engine (README.md), in workbox's own
// terms: only an engine's implementation knows its commands. Nothing blocks:
// each operation is a job (enginejob.h), returned before it starts, which the
// caller starts and which deletes itself after its result unless told not to
// (KJob::setAutoDelete).
#pragma once

#include "enginejob.h"

#include <QMap>

class Engine
{
public:
    virtual ~Engine() = default;

    virtual ListJob *list() = 0;

    // Creates the workbox from image, with its home volume and, with
    // hasSudo, sudo without a password for the user. Leaves it stopped, and
    // nothing of it when it fails.
    virtual EngineJob *create(const Workbox &workbox, const QString &image) = 0;

    virtual EngineJob *start(const QString &id) = 0;

    virtual EngineJob *stop(const QString &id) = 0;

    // Stops the workbox if it runs, and removes it and its home volume.
    virtual EngineJob *remove(const QString &id) = 0;

    // Runs a login shell in the running workbox, as the user in their home
    // or, with asRoot, as root in /root, on this process's terminal (its
    // standard input, output and error), with environment added to the
    // shell's. The job ends when the shell exits, with its exit status.
    virtual EnterJob *enter(const QString &id, const QMap<QString, QString> &environment, bool asRoot) = 0;
};
