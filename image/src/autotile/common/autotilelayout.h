#pragma once

#include <KDirWatch>
#include <KSharedConfig>

#include <QObject>
#include <QString>
#include <QVariantList>

// Autotiling's state, for the tray applet and the KCM, all of it in kwinrc:
// whether the KWin script is on, whether the applet shows its icon, the gap
// between windows and the layout. The file is watched: System Settings > KWin
// Scripts turns the script on and off too, without KConfig's change
// notification. QML creates it as AutotileLayout (com.brescaos.autotile,
// autotilelayoutqml.h).
class AutotileLayout : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString layout READ layout WRITE setLayout NOTIFY layoutChanged)
    // Between windows, in pixels, from 0 to maxGap.
    Q_PROPERTY(int gap READ gap WRITE setGap NOTIFY gapChanged)
    Q_PROPERTY(int defaultGap READ defaultGap CONSTANT)
    Q_PROPERTY(int maxGap READ maxGap CONSTANT)
    // Whether the tray applet shows its icon: off unless asked for.
    Q_PROPERTY(bool showInTray READ showInTray WRITE setShowInTray NOTIFY showInTrayChanged)
    // { name, label, description, icon }, in script/main.js's LAYOUTS order
    // (tests/qml/tst_script.qml); icon is a file's name in qml/icons/.
    Q_PROPERTY(QVariantList layouts READ layouts CONSTANT)

public:
    explicit AutotileLayout(QObject *parent = nullptr);

    bool enabled() const;
    QString layout() const;
    int gap() const;
    int defaultGap() const;
    int maxGap() const;
    bool showInTray() const;
    QVariantList layouts() const;

    // Loads or unloads the script. Unloaded, it can't undo what it did to KWin's
    // running options (edge snapping), so KWin reads them back too.
    void setEnabled(bool enabled);
    // Saves it and has KWin reload its settings, which is when the script reads it.
    void setLayout(const QString &layout);
    // As the layout: KWin reloads its settings and the script reads it.
    void setGap(int gap);
    void setShowInTray(bool show);

Q_SIGNALS:
    void enabledChanged();
    void layoutChanged();
    void gapChanged();
    void showInTrayChanged();

private:
    void reload();

    KSharedConfig::Ptr m_config;
    KDirWatch m_watch;
    bool m_enabled = false;
    QString m_layout;
    int m_gap = 0;
    bool m_showInTray = false;
};
