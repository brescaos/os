// A distrobox command as a KJob, so Plasma shows its progress
// (KUiServerV2JobTracker, runner.cpp): with no percentage its bar is
// indeterminate, and with no capabilities it can't be cancelled or paused (a
// distrobox created halfway would be left behind). It fails with what the
// command said as its errorText().
#pragma once

#include "distroboxcommands.h"

#include <KJob>

class DistroboxJob : public KJob
{
    Q_OBJECT

public:
    // title: what the progress notification says while it runs.
    DistroboxJob(DistroboxCommands::Command command, const QString &name, const QString &title, QObject *parent = nullptr);

    void start() override;

private:
    DistroboxCommands::Command m_command;
    QString m_name;
    QString m_title;
};
