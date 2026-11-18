// The plugin Qt loads Hexa's QtWidgets style from, by its key: Hexa
// (kdeglobals' widgetStyle, System Settings > Application Style).

#include "hexastyle.h"

#include <QStylePlugin>

class HexaStylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QStyleFactoryInterface_iid FILE "hexastyle.json")

public:
    QStyle *create(const QString &key) override
    {
        return key.compare(QLatin1String("hexa"), Qt::CaseInsensitive) == 0 ? new HexaStyle : nullptr;
    }
};

#include "hexastyleplugin.moc"
