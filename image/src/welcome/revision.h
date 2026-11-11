// Whether the welcome opens at login: when this BrescaOS version's revision of
// it is newer than the one the user was last shown (README, "When it opens").
#pragma once

namespace Revision
{
// Raised by a BrescaOS version whose welcome every user sees again at their
// next login. Never lowered.
inline constexpr int current = 1;

// The revision the user was last shown at login, 0 if none:
// ~/.local/state/bresca-welcomerc, [General] ShownRevision.
int readShown();
void writeShown(int revision);
}
