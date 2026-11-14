#pragma once

#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QTimer>

#include <Plasma/Containment>

// Whether the wallpaper behind a panel is dark, so the panel can choose
// content that reads on it. It looks at the wallpaper of the panel's screen
// as Plasma draws it, whatever the wallpaper plugin, and only where the panel
// is; again only when the wallpaper says it changed (README.md).
class WallpaperContrast : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    // The panel's containment (Plasmoid, in its QML).
    Q_PROPERTY(QObject *panel READ panel WRITE setPanel NOTIFY panelChanged)
    // The panel's size across its edge, in pixels.
    Q_PROPERTY(qreal thickness READ thickness WRITE setThickness NOTIFY thicknessChanged)
    // False until the wallpaper has been looked at: isDark says nothing yet.
    Q_PROPERTY(bool isValid READ isValid NOTIFY isValidChanged)
    Q_PROPERTY(bool isDark READ isDark NOTIFY isDarkChanged)

public:
    explicit WallpaperContrast(QObject *parent = nullptr);

    QObject *panel() const;
    void setPanel(QObject *panel);
    qreal thickness() const;
    void setThickness(qreal thickness);
    bool isValid() const;
    bool isDark() const;

Q_SIGNALS:
    void panelChanged();
    void thicknessChanged();
    void isValidChanged();
    void isDarkChanged();

private:
    void followDesktop();
    void followWallpaper();
    void grabWallpaper();
    void readGrab();
    QRect panelArea(const QSize &grabSize) const;
    void setValid(bool isValid);

    QPointer<Plasma::Containment> m_panel;
    QPointer<Plasma::Containment> m_desktop;
    QPointer<QQuickItem> m_wallpaper;
    QSharedPointer<QQuickItemGrabResult> m_grab;
    // Gathers a burst of changes into one look at the wallpaper.
    QTimer m_grabTimer;
    qreal m_thickness = 0;
    bool m_isValid = false;
    bool m_isDark = false;
};
