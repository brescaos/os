#include "commands.h"

#include "color.h"

#include <KLocalizedString>

#include <algorithm>

namespace Commands
{

Workbox create(Engine &engine, const QString &id, const std::optional<QString> &name, const std::optional<QString> &color, const QString &image)
{
    if (!isValidId(id)) {
        throw WorkboxError{i18n("invalid id \"%1\": up to 32 lowercase letters (a-z), digits and hyphens, not starting with a hyphen", id)};
    }
    if (name && name->trimmed().isEmpty()) {
        throw WorkboxError{i18n("the name can't be empty")};
    }
    std::optional<QColor> givenColor;
    if (color) {
        givenColor = WorkboxColor::parse(*color);
        if (!givenColor) {
            throw WorkboxError{i18n("invalid color \"%1\": use #rgb, #rrggbb or r,g,b (each 0-255)", *color)};
        }
    }

    QList<QColor> colorsInUse;
    for (const Workbox &workbox : engine.list()) {
        if (workbox.id == id) {
            throw WorkboxError{i18n("a workbox with the id \"%1\" already exists", id)};
        }
        colorsInUse.append(workbox.color);
    }

    const Workbox workbox{.id = id, .name = name.value_or(id), .color = givenColor ? *givenColor : WorkboxColor::pickFromPalette(colorsInUse)};
    engine.create(workbox, image);
    return workbox;
}

Workbox find(Engine &engine, const QString &id)
{
    for (const Workbox &workbox : engine.list()) {
        if (workbox.id == id) {
            return workbox;
        }
    }
    throw WorkboxError{i18n("there's no workbox with the id \"%1\"", id)};
}

QList<Workbox> list(Engine &engine)
{
    QList<Workbox> workboxes = engine.list();
    std::ranges::sort(workboxes, {}, &Workbox::id);
    return workboxes;
}

}
