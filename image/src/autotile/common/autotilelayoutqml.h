#pragma once

#include "autotilelayout.h"

#include <QQmlEngine>

// AutotileLayout in QML (com.brescaos.autotile). The class itself stays out of
// the QML module, in autotile-core, so the tests can link it.
struct AutotileLayoutQml {
    Q_GADGET
    QML_FOREIGN(AutotileLayout)
    QML_NAMED_ELEMENT(AutotileLayout)
};
