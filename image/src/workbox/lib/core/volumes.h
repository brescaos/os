// A workbox's volumes (README.md), as Podman's -v, and which host folders
// it can see: only inside the user's home, never the home itself, nor a
// folder with keys, other apps' settings or anything the host runs code from.
#pragma once

#include "workbox.h"

// A volume as Podman's -v takes it, "[folder|name:]target[:options]", in
// workbox's terms: folder, relative to the current folder or absolute, is
// shared as its real path (its links resolved, so a link can't get round the
// blocked folders); a name instead (no "/"), as Podman's named volume, is a
// volume of the workbox's own, workbox-<id>.<name>; without either, the
// workbox gets an empty folder of its own at target. target is a folder of the workbox's home, absolute or ~/…;
// options, comma-separated, only ro, rw and z (which workbox always adds).
// Throws a WorkboxError saying why it can't be: an invalid name, a folder that
// isn't one, outside home, home, blocked or holding a blocked one, in
// ~/.config or ~/.local without ro; a target that isn't a clean path in home;
// or an option workbox doesn't allow (Z, U, the others).
Volume parseVolume(const QString &argument, const QString &home);
