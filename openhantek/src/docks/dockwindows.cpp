// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>

#include <cmath>

#include "dockwindows.h"

void SetupDockWidget(QDockWidget *dockWindow, QWidget *dockWidget, QLayout *layout) {
  dockWindow->setObjectName(dockWindow->windowTitle());
  dockWindow->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  dockWidget->setLayout(layout);
  dockWidget->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed,
                                   QSizePolicy::DefaultType));
  dockWindow->setWidget(dockWidget);
}
