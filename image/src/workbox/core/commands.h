// What the commands do, on any engine (README.md); main.cpp reads their
// arguments and prints their results. Each throws a WorkboxError for the user.
#pragma once

#include "engine.h"

#include <optional>

namespace Commands
{

// Creates the workbox id from image. Without a name, its name is its id;
// without a color (#rgb, #rrggbb or r,g,b), one from the palette.
Workbox create(Engine &engine, const QString &id, const std::optional<QString> &name, const std::optional<QString> &color, const QString &image);

Workbox find(Engine &engine, const QString &id);

// Every workbox, by id.
QList<Workbox> list(Engine &engine);

}
