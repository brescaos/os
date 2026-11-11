#include "revision.h"

#include <KConfigGroup>
#include <KSharedConfig>

namespace
{
const QString shownKey = QStringLiteral("ShownRevision");

KConfigGroup openState()
{
    return KSharedConfig::openStateConfig(QStringLiteral("bresca-welcomerc"))->group(QStringLiteral("General"));
}
}

int Revision::readShown()
{
    return openState().readEntry(shownKey, 0);
}

void Revision::writeShown(int revision)
{
    KConfigGroup state = openState();
    state.writeEntry(shownKey, revision);
    state.sync();
}
