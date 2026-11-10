// System Settings > BrescaOS Updates. The page is ui/main.qml; the metadata,
// kcm_brescaos_updates.json, is kcm_brescaos_updates.json.in translated by
// image/build/compile_translations.sh.
#include <KPluginFactory>
#include <KQuickConfigModule>

class UpdatesKcm : public KQuickConfigModule
{
    Q_OBJECT

public:
    UpdatesKcm(QObject *parent, const KPluginMetaData &metaData)
        : KQuickConfigModule(parent, metaData)
    {
    }
};

K_PLUGIN_CLASS_WITH_JSON(UpdatesKcm, "kcm_brescaos_updates.json")

#include "kcm.moc"
