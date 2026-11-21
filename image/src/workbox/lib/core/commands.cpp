#include "commands.h"

#include "color.h"
#include "volumes.h"

#include <KLocalizedString>

#include <QDir>

#include <algorithm>

namespace Commands
{

CreateJob::CreateJob(Engine &engine, const QString &id, const CreateOptions &options, const QString &image, QObject *parent)
    : EngineJob(parent)
    , m_engine(engine)
    , m_options(options)
    , m_image(image)
    , m_workbox{.id = id, .name = options.name.value_or(id), .hasSudo = options.hasSudo, .hasWayland = options.hasWayland}
{
}

void CreateJob::start()
{
    // After start() returns, as a KJob's result always comes.
    QMetaObject::invokeMethod(
        this,
        [this] {
            if (!isValidId(m_workbox.id)) {
                fail(i18n("invalid id \"%1\": up to 32 lowercase letters (a-z), digits and hyphens, not starting with a hyphen", m_workbox.id));
                return;
            }
            if (m_options.name && m_options.name->trimmed().isEmpty()) {
                fail(i18n("the name can't be empty"));
                return;
            }
            if (m_options.color) {
                const std::optional<QColor> givenColor = WorkboxColor::parse(*m_options.color);
                if (!givenColor) {
                    fail(i18n("invalid color \"%1\": use #rgb, #rrggbb or r,g,b (each 0-255)", *m_options.color));
                    return;
                }
                m_workbox.color = *givenColor;
            }
            if (m_options.hasWayland && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
                fail(i18n("can't give it Wayland: there's no Wayland session here (WAYLAND_DISPLAY)"));
                return;
            }
            for (const QString &argument : std::as_const(m_options.volumes)) {
                Volume volume;
                try {
                    volume = parseVolume(argument, QDir::homePath());
                } catch (const WorkboxError &error) {
                    fail(error.message);
                    return;
                }
                if (std::ranges::any_of(m_workbox.volumes, [&volume](const Volume &other) {
                        return other.target == volume.target;
                    })) {
                    fail(i18n("two volumes are at %1", volume.target));
                    return;
                }
                m_workbox.volumes.append(volume);
            }
            ListJob *listJob = m_engine.list();
            runStep(listJob, [this, listJob] {
                createFrom(listJob->workboxes());
            });
        },
        Qt::QueuedConnection);
}

void CreateJob::createFrom(const QList<Workbox> &workboxes)
{
    QList<QColor> colorsInUse;
    for (const Workbox &workbox : workboxes) {
        if (workbox.id == m_workbox.id) {
            fail(i18n("a workbox with the id \"%1\" already exists", m_workbox.id));
            return;
        }
        colorsInUse.append(workbox.color);
    }
    if (!m_options.color) {
        m_workbox.color = WorkboxColor::pickFromPalette(colorsInUse);
    }
    runStep(m_engine.create(m_workbox, m_image), [this] {
        emitResult();
    });
}

void CreateJob::runStep(EngineJob *job, const std::function<void()> &then)
{
    m_step = job;
    job->setOutputToTerminal(isOutputToTerminal());
    connect(job, &EngineJob::output, this, &EngineJob::output);
    connect(job, &KJob::result, this, [this, job, then] {
        m_step = nullptr;
        if (job->error()) {
            fail(job->errorText());
            return;
        }
        then();
    });
    job->start();
}

bool CreateJob::doKill()
{
    return !m_step || m_step->kill();
}

Workbox find(const QList<Workbox> &workboxes, const QString &id)
{
    for (const Workbox &workbox : workboxes) {
        if (workbox.id == id) {
            return workbox;
        }
    }
    throw WorkboxError{i18n("there's no workbox with the id \"%1\"", id)};
}

QList<Workbox> sortedById(QList<Workbox> workboxes)
{
    std::ranges::sort(workboxes, {}, &Workbox::id);
    return workboxes;
}

}
