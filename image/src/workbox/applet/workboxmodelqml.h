#pragma once

#include "workboxmodel.h"

#include <QQmlEngine>

// WorkboxModel in QML (com.brescaos.workbox). The class itself stays out of
// the QML module, in workbox-applet-model, so the tests can link it.
struct WorkboxModelQml {
    Q_GADGET
    QML_FOREIGN(WorkboxModel)
    QML_NAMED_ELEMENT(WorkboxModel)
};
