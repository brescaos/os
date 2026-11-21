// workbox's settings (README.md): the image's, with the user's on top.
#pragma once

#include <QString>

struct Settings {
    QString engine;
    QString image;
};

// Each key from userFile if it has it, else from systemFile. Throws a
// WorkboxError when neither has one.
Settings readSettings(const QString &systemFile, const QString &userFile);
