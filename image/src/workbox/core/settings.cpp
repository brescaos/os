#include "settings.h"

#include "workbox.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

Settings readSettings(const QString &systemFile, const QString &userFile)
{
    // KConfig reads the sources it's given only when it cascades; with an
    // absolute path, it doesn't add any of its own.
    KConfig config(userFile, KConfig::NoGlobals);
    config.addConfigSources({systemFile});
    const KConfigGroup general = config.group(QStringLiteral("General"));

    const auto readRequiredEntry = [&](const char *key) {
        const QString value = general.readEntry(key, QString());
        if (value.isEmpty()) {
            throw WorkboxError{i18n("no %1 in %2 or %3", QString::fromLatin1(key), systemFile, userFile)};
        }
        return value;
    };
    return {.engine = readRequiredEntry("engine"), .image = readRequiredEntry("image")};
}
