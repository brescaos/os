#include "settings.h"

#include "workbox.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <QStandardPaths>

QStringList settingsFiles()
{
    return {QStringLiteral(SETTINGS_FILE),
            QStringLiteral(DISTRIBUTION_SETTINGS_FILE),
            QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/workbox/workbox.conf")};
}

Settings readSettings(const QStringList &files)
{
    // KConfig reads the sources it's given only when it cascades; with an
    // absolute path, it doesn't add any of its own.
    KConfig config(files.last(), KConfig::NoGlobals);
    config.addConfigSources(files.mid(0, files.size() - 1));
    const KConfigGroup general = config.group(QStringLiteral("General"));

    const auto readRequiredEntry = [&](const char *key) {
        const QString value = general.readEntry(key, QString());
        if (value.isEmpty()) {
            throw WorkboxError{i18n("no %1 in %2", QString::fromLatin1(key), files.join(QStringLiteral(", ")))};
        }
        return value;
    };
    return {.engine = readRequiredEntry("engine"), .image = readRequiredEntry("image")};
}
