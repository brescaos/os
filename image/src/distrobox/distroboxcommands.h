// The podman and distrobox commands BrescaOS runs without a terminal
// (README.md), shared by the tray applet's list (distroboxmodel.h) and the
// KRunner plugin (krunner/).
#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <functional>

class QObject;

// Runs program with arguments, then calls done with whether it exited with 0,
// its output and its errors (or why it didn't start).
void runProgram(QObject *context,
                const QString &program,
                const QStringList &arguments,
                const std::function<void(bool hasSucceeded, const QString &output, const QString &errors)> &done);

namespace DistroboxCommands
{

struct ListedDistrobox {
    QString name;
    bool isRunning = false;
};

// podman's arguments to list every distrobox of the user, running or not, as
// JSON, and that JSON's distroboxes, in podman's order.
QStringList listArguments();
QList<ListedDistrobox> parseListing(const QString &json);

// Called when a command ends: whether it succeeded and, if it didn't, what it
// said.
using Finished = std::function<void(bool hasSucceeded, const QString &errors)>;
using Command = void (*)(QObject *context, const QString &name, const Finished &finished);

// With Distrobox's default image, downloaded without asking the first time.
void create(QObject *context, const QString &name, const Finished &finished);
// Returns once the first start's setup is done, which can take a few minutes.
void start(QObject *context, const QString &name, const Finished &finished);
void stop(QObject *context, const QString &name, const Finished &finished);
// Without asking, even if it's running.
void remove(QObject *context, const QString &name, const Finished &finished);

}
