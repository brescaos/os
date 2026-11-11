#pragma once

#include "distroboxmodel.h"

#include <QQmlEngine>

// DistroboxModel in QML (com.brescaos.distrobox). The class itself stays out of
// the QML module, in distrobox-core, so the tests can link it.
struct DistroboxModelQml {
    Q_GADGET
    QML_FOREIGN(DistroboxModel)
    QML_NAMED_ELEMENT(DistroboxModel)
};
