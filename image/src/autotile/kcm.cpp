// System Settings > Window Management > Automatic Tiling. The page (ui/main.qml)
// keeps the pending changes, around an AutotileLayout: these hand it the module's
// buttons.
#include <KPluginFactory>
#include <KQuickConfigModule>

class AutotileKcm : public KQuickConfigModule
{
    Q_OBJECT

public:
    AutotileKcm(QObject *parent, const KPluginMetaData &data)
        : KQuickConfigModule(parent, data)
    {
        setButtons(Apply | Default);
    }

    void load() override
    {
        Q_EMIT loadRequested();
    }

    void save() override
    {
        Q_EMIT saveRequested();
    }

    void defaults() override
    {
        Q_EMIT defaultsRequested();
    }

Q_SIGNALS:
    void loadRequested();
    void saveRequested();
    void defaultsRequested();
};

// The metadata, kcm_brescaos_autotile.json, is kcm_brescaos_autotile.json.in
// translated by image/build/compile_translations.sh.
K_PLUGIN_CLASS_WITH_JSON(AutotileKcm, "kcm_brescaos_autotile.json")

#include "kcm.moc"
