// workbox's settings (README.md), workbox.conf on three levels, each key
// from the most specific that has it: workbox's own, a distribution's, the
// user's.
#pragma once

#include <QStringList>

struct Settings {
    QString engine;
    QString image;
};

// The three levels, the most general first.
QStringList settingsFiles();

// Each key from the last of files that has it. Throws a WorkboxError when
// none has one.
Settings readSettings(const QStringList &files);
