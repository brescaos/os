// The engine workbox.conf names (README.md): today only podman.
#pragma once

#include "engine.h"

#include <memory>

// Throws a WorkboxError for an engine workbox doesn't have.
std::unique_ptr<Engine> engineNamed(const QString &name);
