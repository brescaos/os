#include "autotilelayout.h"

#include <KConfigGroup>
#include <KLocalizedString>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QStandardPaths>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace
{
const auto configName = u"kwinrc"_s;
// KWin's group for the script's settings and its "<id>Enabled" key.
const auto scriptGroup = u"Script-com.brescaos.autotile"_s;
const auto enabledKey = "com.brescaos.autotileEnabled";
// As DEFAULT_LAYOUT in script/main.js (tests/qml/tst_script.qml).
const auto defaultLayout = u"binaryTree"_s;
// As DEFAULT_GAP in script/main.js (tests/qml/tst_script.qml).
constexpr int defaultGapValue = 8;
constexpr int maxGapValue = 64;

void callKWin(const QString &path, const QString &interface, const QString &method)
{
    QDBusConnection::sessionBus().asyncCall(QDBusMessage::createMethodCall(u"org.kde.KWin"_s, path, interface, method));
}
}

AutotileLayout::AutotileLayout(QObject *parent)
    : QObject(parent)
    , m_config(KSharedConfig::openConfig(configName))
{
    const auto changed = [this] {
        m_config->reparseConfiguration();
        reload();
    };
    connect(&m_watch, &KDirWatch::dirty, this, changed);
    connect(&m_watch, &KDirWatch::created, this, changed);
    m_watch.addFile(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u'/' + configName);
    reload();
}

bool AutotileLayout::enabled() const
{
    return m_enabled;
}

QString AutotileLayout::layout() const
{
    return m_layout;
}

int AutotileLayout::gap() const
{
    return m_gap;
}

int AutotileLayout::defaultGap() const
{
    return defaultGapValue;
}

int AutotileLayout::maxGap() const
{
    return maxGapValue;
}

bool AutotileLayout::showInTray() const
{
    return m_showInTray;
}

QVariantList AutotileLayout::layouts() const
{
    return {
        QVariantMap{{u"name"_s, u"binaryTree"_s}, {u"label"_s, i18n("Binary tree")},
                    {u"description"_s, i18n("Each new window splits the active one in two")},
                    {u"icon"_s, u"layout-binary-tree-symbolic.svg"_s}},
        QVariantMap{{u"name"_s, u"master"_s}, {u"label"_s, i18n("Master")},
                    {u"description"_s, i18n("One main window on the left, the others stacked on the right")},
                    {u"icon"_s, u"layout-master-symbolic.svg"_s}},
        QVariantMap{{u"name"_s, u"grid"_s}, {u"label"_s, i18n("Grid")},
                    {u"description"_s, i18n("All windows the same size, in rows")},
                    {u"icon"_s, u"layout-grid-symbolic.svg"_s}},
    };
}

void AutotileLayout::setEnabled(bool enabled)
{
    if (enabled == m_enabled) {
        return;
    }
    m_config->group(u"Plugins"_s).writeEntry(enabledKey, enabled);
    m_config->sync();
    reload();
    // Loads what's on and unloads what isn't.
    callKWin(u"/Scripting"_s, u"org.kde.kwin.Scripting"_s, u"start"_s);
    if (!enabled) {
        callKWin(u"/KWin"_s, u"org.kde.KWin"_s, u"reconfigure"_s);
    }
}

void AutotileLayout::setLayout(const QString &layout)
{
    m_config->group(scriptGroup).writeEntry("Layout", layout);
    m_config->sync();
    reload();
    callKWin(u"/KWin"_s, u"org.kde.KWin"_s, u"reconfigure"_s);
}

void AutotileLayout::setGap(int gap)
{
    m_config->group(scriptGroup).writeEntry("Gap", gap);
    m_config->sync();
    reload();
    callKWin(u"/KWin"_s, u"org.kde.KWin"_s, u"reconfigure"_s);
}

void AutotileLayout::setShowInTray(bool show)
{
    m_config->group(scriptGroup).writeEntry("ShowInTray", show);
    m_config->sync();
    reload();
}

void AutotileLayout::reload()
{
    const int gap = m_config->group(scriptGroup).readEntry("Gap", defaultGapValue);
    if (gap != m_gap) {
        m_gap = gap;
        Q_EMIT gapChanged();
    }
    const bool showInTray = m_config->group(scriptGroup).readEntry("ShowInTray", false);
    if (showInTray != m_showInTray) {
        m_showInTray = showInTray;
        Q_EMIT showInTrayChanged();
    }
    const bool enabled = m_config->group(u"Plugins"_s).readEntry(enabledKey, false);
    if (enabled != m_enabled) {
        m_enabled = enabled;
        Q_EMIT enabledChanged();
    }
    // One no longer known, as the script takes it (savedLayout in script/main.js).
    QString layout = m_config->group(scriptGroup).readEntry("Layout", defaultLayout);
    const QVariantList known = layouts();
    if (std::none_of(known.begin(), known.end(), [&layout](const QVariant &entry) {
            return entry.toMap().value(u"name"_s).toString() == layout;
        })) {
        layout = defaultLayout;
    }
    if (layout != m_layout) {
        m_layout = layout;
        Q_EMIT layoutChanged();
    }
}
