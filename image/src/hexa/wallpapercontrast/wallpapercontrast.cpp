#include "wallpapercontrast.h"

#include "luminance.h"

#include <Plasma/Corona>

#include <chrono>

using namespace std::chrono_literals;

// Enough to average the panel's strip (a 26 px bar on a 1920 px screen is
// 3 rows), and cheap to draw.
static const int GrabWidth = 256;

WallpaperContrast::WallpaperContrast(QObject *parent)
    : QObject(parent)
{
    m_grabTimer.setSingleShot(true);
    m_grabTimer.setInterval(250ms);
    connect(&m_grabTimer, &QTimer::timeout, this, &WallpaperContrast::grabWallpaper);
}

QObject *WallpaperContrast::panel() const
{
    return m_panel;
}

void WallpaperContrast::setPanel(QObject *panel)
{
    auto *containment = qobject_cast<Plasma::Containment *>(panel);
    if (m_panel == containment) {
        return;
    }
    if (m_panel) {
        disconnect(m_panel->corona(), nullptr, this, nullptr);
        disconnect(m_panel, nullptr, this, nullptr);
        disconnect(m_panel, nullptr, &m_grabTimer, nullptr);
    }
    m_panel = containment;
    if (m_panel) {
        // The screens' desktops change with the screens and the activity.
        connect(m_panel->corona(), &Plasma::Corona::screenOwnerChanged, this, &WallpaperContrast::followDesktop);
        connect(m_panel, &Plasma::Containment::screenChanged, this, &WallpaperContrast::followDesktop);
        connect(m_panel, &Plasma::Containment::locationChanged, &m_grabTimer, qOverload<>(&QTimer::start));
    }
    followDesktop();
    Q_EMIT panelChanged();
}

qreal WallpaperContrast::thickness() const
{
    return m_thickness;
}

void WallpaperContrast::setThickness(qreal thickness)
{
    if (m_thickness == thickness) {
        return;
    }
    m_thickness = thickness;
    m_grabTimer.start();
    Q_EMIT thicknessChanged();
}

bool WallpaperContrast::isValid() const
{
    return m_isValid;
}

bool WallpaperContrast::isDark() const
{
    return m_isDark;
}

// Follows the desktop of the panel's screen: the one that draws the wallpaper
// behind it.
void WallpaperContrast::followDesktop()
{
    Plasma::Containment *desktop = nullptr;
    // A desktop of another activity has no screen (-1), as a panel may.
    if (m_panel && m_panel->screen() >= 0) {
        const QList<Plasma::Containment *> containments = m_panel->corona()->containments();
        for (Plasma::Containment *containment : containments) {
            if (containment->containmentType() == Plasma::Containment::Type::Desktop
                && containment->screen() == m_panel->screen()) {
                desktop = containment;
                break;
            }
        }
    }
    if (m_desktop != desktop) {
        if (m_desktop) {
            disconnect(m_desktop, nullptr, this, nullptr);
        }
        m_desktop = desktop;
        if (m_desktop) {
            // Another wallpaper plugin is another item.
            connect(m_desktop, &Plasma::Containment::wallpaperGraphicsObjectChanged, this,
                    &WallpaperContrast::followWallpaper);
        }
    }
    followWallpaper();
}

void WallpaperContrast::followWallpaper()
{
    // The property: its getter isn't part of Plasma's public API.
    QQuickItem *wallpaper =
        m_desktop ? qobject_cast<QQuickItem *>(m_desktop->property("wallpaperGraphicsObject").value<QObject *>())
                  : nullptr;
    if (m_wallpaper == wallpaper) {
        return;
    }
    if (m_wallpaper) {
        disconnect(m_wallpaper, nullptr, &m_grabTimer, nullptr);
    }
    m_wallpaper = wallpaper;
    if (!m_wallpaper) {
        m_grabTimer.stop();
        setValid(false);
        return;
    }
    // Plasma's wallpapers emit accentColorChanged() whenever what they show
    // changes, for Plasma's accent color from the wallpaper. By name: the
    // item's class (WallpaperItem) is private to Plasma.
    connect(m_wallpaper, SIGNAL(accentColorChanged()), &m_grabTimer, SLOT(start()));
    connect(m_wallpaper, &QQuickItem::widthChanged, &m_grabTimer, qOverload<>(&QTimer::start));
    connect(m_wallpaper, &QQuickItem::heightChanged, &m_grabTimer, qOverload<>(&QTimer::start));
    m_grabTimer.start();
}

void WallpaperContrast::grabWallpaper()
{
    if (!m_wallpaper || m_wallpaper->width() <= 0 || m_wallpaper->height() <= 0) {
        return;
    }
    const int grabHeight = qMax(1, qRound(GrabWidth * m_wallpaper->height() / m_wallpaper->width()));
    // Null while the wallpaper isn't on a visible window: it says when it shows.
    m_grab = m_wallpaper->grabToImage(QSize(GrabWidth, grabHeight));
    if (m_grab) {
        connect(m_grab.get(), &QQuickItemGrabResult::ready, this, &WallpaperContrast::readGrab);
    }
}

void WallpaperContrast::readGrab()
{
    const QImage wallpaperImage = m_grab->image();
    m_grab.clear();
    if (wallpaperImage.isNull() || !m_panel) {
        return;
    }
    const bool isDark = isDarkArea(wallpaperImage, panelArea(wallpaperImage.size()));
    if (m_isDark != isDark) {
        m_isDark = isDark;
        Q_EMIT isDarkChanged();
    }
    setValid(true);
}

// The panel's strip in a grab of the wallpaper, which fills the screen: along
// the panel's edge, as thick as the panel at the grab's scale.
QRect WallpaperContrast::panelArea(const QSize &grabSize) const
{
    const bool isVertical =
        m_panel->location() == Plasma::Types::LeftEdge || m_panel->location() == Plasma::Types::RightEdge;
    const int grabLength = isVertical ? grabSize.width() : grabSize.height();
    const qreal screenLength = isVertical ? m_panel->screenGeometry().width() : m_panel->screenGeometry().height();
    const int thickness = qBound(1, qRound(m_thickness * grabLength / screenLength), grabLength);
    switch (m_panel->location()) {
    case Plasma::Types::BottomEdge:
        return QRect(0, grabSize.height() - thickness, grabSize.width(), thickness);
    case Plasma::Types::LeftEdge:
        return QRect(0, 0, thickness, grabSize.height());
    case Plasma::Types::RightEdge:
        return QRect(grabSize.width() - thickness, 0, thickness, grabSize.height());
    default:
        return QRect(0, 0, grabSize.width(), thickness);
    }
}

void WallpaperContrast::setValid(bool isValid)
{
    if (m_isValid == isValid) {
        return;
    }
    m_isValid = isValid;
    Q_EMIT isValidChanged();
}
