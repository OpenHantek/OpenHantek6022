// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>

#include <cmath>

#include "analyse/enums.h"
#include "hantekdso/enums.h"
#include "dockwindows.h"

void SetupDockWidget(QDockWidget *dockWindow, QWidget *dockWidget, QLayout *layout) {
    dockWindow->setObjectName(dockWindow->windowTitle());
    dockWindow->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dockWidget->setLayout(layout);
    dockWidget->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed, QSizePolicy::DefaultType));
    dockWindow->setWidget(dockWidget);
}

void registerDockMetaTypes() {
    qRegisterMetaType<Dso::TriggerMode>();
    qRegisterMetaType<Dso::MathMode>();
    qRegisterMetaType<Dso::Slope>();
    qRegisterMetaType<Dso::Coupling>();
    qRegisterMetaType<Dso::GraphFormat>();
    qRegisterMetaType<Dso::ChannelMode>();
    qRegisterMetaType<Dso::WindowFunction>();
    qRegisterMetaType<Dso::InterpolationMode>();
    qRegisterMetaType<std::vector<unsigned> >();
    qRegisterMetaType<std::vector<double> >();
}
