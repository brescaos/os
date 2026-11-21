#include "engines.h"

#include "podmanengine.h"

#include <KLocalizedString>

std::unique_ptr<Engine> engineNamed(const QString &name)
{
    if (name != QLatin1String("podman")) {
        throw WorkboxError{i18n("unknown engine \"%1\" in workbox.conf: only podman", name)};
    }
    return std::make_unique<PodmanEngine>();
}
