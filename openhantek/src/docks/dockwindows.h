// SPDX-License-Identifier: GPL-2.0+

#pragma once


#include <QDockWidget>
#include <QLayout>

void SetupDockWidget(QDockWidget *dockWindow, QWidget *dockWidget, QLayout *layout);

#include "HorizontalDock.h"
#include "SpectrumDock.h"
#include "TriggerDock.h"
#include "VoltageDock.h"
